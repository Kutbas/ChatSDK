#include "../include/DeepSeekProvider.h"
#include "../include/util/myLog.h"
#include <jsoncpp/json/json.h>
#include <sstream>
#include <httplib.h>

namespace ai_chat_sdk
{
    // DeepSeekProvider 类
    bool DeepSeekProvider::initModel(const std::map<std::string, std::string> &modelConfig)
    {
        // 初始化API Key
        auto it = modelConfig.find("api_key");
        if (it == modelConfig.end())
        {
            ERR("DeepSeekProvider initModel api_key not found");
            return false;
        }
        else
        {
            _apiKey = it->second;
        }

        // 初始化Base URL
        it = modelConfig.find("endpoint");
        if (it == modelConfig.end())
        {
            _endpoint = "https://api.deepseek.com";
        }
        else
        {
            _endpoint = it->second;
        }

        _isAvailable = true;
        INFO("DeepSeekProvider initModel success, endpoint: {}", _endpoint);
        return true;
    }

    // 检测模型是否可用
    bool DeepSeekProvider::isAvailable() const
    {
        return _isAvailable;
    }

    // 获取模型名称
    std::string DeepSeekProvider::getModelName() const
    {
        return "deepseek-chat";
    }

    // 获取模型的描述信息
    std::string DeepSeekProvider::getModelDesc() const
    {
        return "一款实用性强、中文优化的通用对话助手，适合日常问答与创作";
    }

    std::string DeepSeekProvider::sendMessage(const std::vector<Message> &messages,
                                              const std::map<std::string, std::string> &requestParam)
    {
        // 1. 检测模型是否可用 (API Key 是否已初始化)
        if (!isAvailable())
        {
            ERR("DeepSeekProvider sendMessage: Model is not available (not initialized).");
            return "";
        }

        // 2. 准备请求参数 (设置默认值)
        double temperature = 0.7;
        int maxTokens = 2048;

        // 从 requestParam 中提取用户自定义参数
        if (requestParam.find("temperature") != requestParam.end())
        {
            try
            {
                temperature = std::stod(requestParam.at("temperature"));
            }
            catch (...)
            {
                WARN("Invalid temperature param, using default 0.7");
            }
        }
        if (requestParam.find("max_tokens") != requestParam.end())
        {
            try
            {
                maxTokens = std::stoi(requestParam.at("max_tokens"));
            }
            catch (...)
            {
                WARN("Invalid max_tokens param, using default 2048");
            }
        }

        // 3. 构造历史消息列表 (Context)
        // DeepSeek 要求 messages 为 JSON 数组
        Json::Value messageArray(Json::arrayValue);
        for (const auto &msg : messages)
        {
            Json::Value messageObject;
            messageObject["role"] = msg._role; // user, assistant, system
            messageObject["content"] = msg._content;
            messageArray.append(messageObject);
        }

        // 4. 构造完整的请求体 (Request Body)
        Json::Value requestBody;
        requestBody["model"] = getModelName(); // deepseek-chat
        requestBody["messages"] = messageArray;
        requestBody["temperature"] = temperature;
        requestBody["max_tokens"] = maxTokens;
        requestBody["stream"] = false; // 全量返回模式

        // 5. 序列化: 将 JSON 对象转为字符串 (JSON String)
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = ""; // 紧凑格式，无缩进
        std::string requestBodyStr = Json::writeString(writerBuilder, requestBody);

        INFO("DeepSeekProvider sendMessage Request Body: {}", requestBodyStr);

        // 6. 创建 HTTP 客户端
        // _endpoint 例如: https://api.deepseek.com
        httplib::Client client(_endpoint.c_str());

        // 设置超时时间
        client.set_connection_timeout(30, 0); // 连接超时 30秒
        client.set_read_timeout(60, 0);       // 读取超时 60秒 (大模型生成较慢)

        // 设置请求头
        httplib::Headers headers = {
            {"Authorization", "Bearer " + _apiKey},
            {"Content-Type", "application/json"}};

        // 7. 发送 POST 请求
        // 路径为 /chat/completions (DeepSeek 官方兼容 OpenAI 接口)
        // 注意：有些官方文档可能建议使用 /v1/chat/completions，请根据实际情况调整
        auto response = client.Post("/v1/chat/completions", headers, requestBodyStr, "application/json");

        // 8. 检查响应状态
        if (!response)
        {
            ERR("DeepSeekProvider sendMessage POST request failed (Network Error).");
            return "";
        }

        if (response->status != 200)
        {
            ERR("DeepSeekProvider sendMessage Failed. Status: {}, Body: {}", response->status, response->body);
            return "";
        }

        INFO("DeepSeekProvider Request Success. Status: {}", response->status);

        // 9. 解析响应体 (反序列化)
        Json::Value responseBody;
        Json::CharReaderBuilder readerBuilder;
        std::string parseError;
        std::istringstream responseStream(response->body);

        // 使用 parseFromStream 解析 JSON 字符串
        if (Json::parseFromStream(readerBuilder, responseStream, &responseBody, &parseError))
        {
            // 提取 content 字段
            // 结构路径: choices[0] -> message -> content
            if (responseBody.isMember("choices") &&
                responseBody["choices"].isArray() &&
                !responseBody["choices"].empty())
            {

                auto choice = responseBody["choices"][0];
                if (choice.isMember("message") && choice["message"].isMember("content"))
                {
                    std::string replyContent = choice["message"]["content"].asString();
                    INFO("DeepSeekProvider Response Text: {}", replyContent);
                    return replyContent;
                }
            }

            ERR("DeepSeekProvider: 'content' field not found in response.");
        }
        else
        {
            ERR("DeepSeekProvider: JSON parse failed. Error: {}", parseError);
        }

        return "Error: Failed to parse DeepSeek response.";
    }

    // 发送消息 - 增量返回 - 流式响应
    std::string DeepSeekProvider::sendMessageStream(
        const std::vector<Message> &messages,
        const std::map<std::string, std::string> &requestParam,
        std::function<void(const std::string &, bool)> callback)
    {

        // 1. 检测模型是否可用
        if (!isAvailable())
        {
            ERR("DeepSeekProvider sendMessageStream: Model not available.");
            return "";
        }

        // 2. 准备请求参数
        double temperature = 0.7;
        int maxTokens = 2048;
        // ... (参数解析逻辑同 sendMessage，略)

        // 3. 构造请求体 (Request Body)
        // 注意：这里必须显式开启 stream = true
        Json::Value requestBody;
        requestBody["model"] = getModelName();
        requestBody["stream"] = true; // 关键！
        requestBody["temperature"] = temperature;
        requestBody["max_tokens"] = maxTokens;

        // 构造 messages 数组
        Json::Value messageArray(Json::arrayValue);
        for (const auto &msg : messages)
        {
            Json::Value item;
            item["role"] = msg._role;
            item["content"] = msg._content;
            messageArray.append(item);
        }
        requestBody["messages"] = messageArray;

        // 序列化
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "";
        std::string requestBodyStr = Json::writeString(writerBuilder, requestBody);

        INFO("DeepSeek Stream Request: {}", requestBodyStr);

        // 4. 创建 HTTP 客户端
        httplib::Client client(_endpoint.c_str());
        client.set_connection_timeout(30, 0);
        // 流式响应持续时间可能很长，设置读取超时为 300秒
        client.set_read_timeout(300, 0);

        // 设置请求头
        httplib::Headers headers = {
            {"Authorization", "Bearer " + _apiKey},
            {"Content-Type", "application/json"},
            {"Accept", "text/event-stream"} // 告诉服务器我们要接收事件流
        };

        // 5. 定义流式处理所需的上下文变量
        std::string buffer;        // 数据缓冲区 (处理粘包/半包)
        bool gotError = false;     // 错误标记
        std::string errorMsg;      // 错误信息
        bool streamFinish = false; // 是否收到 [DONE]
        std::string fullResponse;  // 累积完整回复

        // 6. 构造 Request 对象
        httplib::Request req;
        req.method = "POST";
        req.path = "/chat/completions";
        req.headers = headers;
        req.body = requestBodyStr;

        // 7. 设置响应头处理器 (Response Handler)
        // 在接收到 HTTP 响应头时触发
        req.response_handler = [&](const httplib::Response &res)
        {
            if (res.status != 200)
            {
                gotError = true;
                errorMsg = "HTTP Status: " + std::to_string(res.status);
                ERR("Stream Handshake Failed: {}", errorMsg);
                return false; // 返回 false 终止连接
            }
            return true; // 继续接收 Body
        };

        // 8. 设置内容接收器 (Content Receiver)
        // 每收到一块数据就会触发此回调
        req.content_receiver = [&](const char *data, size_t len, uint64_t /*offset*/, uint64_t /*total*/)
        {
            if (gotError)
                return false;

            // 将新接收的数据追加到缓冲区
            buffer.append(data, len);

            // 循环处理缓冲区中的完整 SSE 消息
            // SSE 规范：每条消息以两个换行符 \n\n 结尾
            size_t pos = 0;
            while ((pos = buffer.find("\n\n")) != std::string::npos)
            {
                // 提取一条完整的消息
                std::string chunk = buffer.substr(0, pos);
                // 从缓冲区移除已处理的部分 (+2 是跳过 \n\n)
                buffer.erase(0, pos + 2);

                // 忽略空行和注释 (以冒号开头的行)
                if (chunk.empty() || chunk[0] == ':')
                    continue;

                // 解析 data: 开头的数据行
                // 格式: "data: {...}"
                const std::string prefix = "data: ";
                if (chunk.compare(0, prefix.size(), prefix) == 0)
                {
                    std::string payload = chunk.substr(prefix.size());

                    // 检查结束标记
                    if (payload == "[DONE]")
                    {
                        streamFinish = true;
                        // 通知上层：对话结束
                        if (callback)
                            callback("", true);
                        return true;
                    }

                    // JSON 反序列化
                    Json::Value jsonResp;
                    Json::CharReaderBuilder reader;
                    std::string errs;
                    std::istringstream ss(payload);

                    if (Json::parseFromStream(reader, ss, &jsonResp, &errs))
                    {
                        // 提取 content
                        // 路径: choices[0].delta.content (注意这里是 delta 不是 message)
                        if (jsonResp.isMember("choices") &&
                            !jsonResp["choices"].empty())
                        {

                            auto choice = jsonResp["choices"][0];
                            if (choice.isMember("delta") && choice["delta"].isMember("content"))
                            {
                                std::string content = choice["delta"]["content"].asString();

                                // 累积完整回复
                                fullResponse += content;

                                // 触发回调，通知上层有新字符生成
                                if (callback)
                                    callback(content, false);
                            }
                        }
                    }
                    else
                    {
                        WARN("SSE JSON Parse Failed: {}", errs);
                    }
                }
            }
            return true; // 继续接收下一块数据
        };

        // 9. 发送请求
        auto res = client.send(req);

        // 10. 处理发送结果
        if (!res)
        {
            ERR("Network Error: {}", to_string(res.error()));
            return "";
        }

        // 11. 兜底检查
        // 如果连接意外断开且没有收到 [DONE]，需要通知上层结束
        if (!streamFinish && !gotError)
        {
            WARN("Stream ended unexpectedly without [DONE]");
            if (callback)
                callback("", true);
        }

        return fullResponse;
    }

} // end ai_chat_sdk
