#include "../include/GeminiProvider.h"
#include "../include/util/myLog.h"

#include <httplib.h>
#include <jsoncpp/json/forwards.h>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>

namespace ai_chat_sdk
{

    bool GeminiProvider::initModel(const std::map<std::string, std::string> &modelConfig)
    {
        // 1. 提取 API Key
        auto it = modelConfig.find("api_key");
        if (it == modelConfig.end())
        {
            ERR("GeminiProvider::initModel: 'api_key' not found in config.");
            return false;
        }
        _apiKey = it->second;

        // 2. 提取 Endpoint (默认使用 Google 官方地址)
        it = modelConfig.find("endpoint");
        if (it == modelConfig.end())
        {
            _endpoint = "https://generativelanguage.googleapis.com";
        }
        else
        {
            _endpoint = it->second;
        }

        // 3. 标记初始化成功
        _isAvailable = true;
        INFO("GeminiProvider::initModel: Init success. Endpoint: {}", _endpoint);
        return true;
    }

    bool GeminiProvider::isAvailable() const
    {
        return _isAvailable;
    }

    std::string GeminiProvider::getModelName() const
    {
        // 指定使用的模型版本
        return "gemini-2.5-flash";
    }

    std::string GeminiProvider::getModelDesc() const
    {
        return "Google Gemini 2.5 Flash: Google 的极速响应模型，专为低延迟、高吞吐量的交互场景设计。";
    }

    // 发送消息 - 全量返回
    std::string GeminiProvider::sendMessage(const std::vector<Message> &messages,
                                            const std::map<std::string, std::string> &requestParam)
    {
        // 1. 检测模型是否可用
        if (!isAvailable())
        {
            ERR("GeminiProvider::sendMessage: model not available");
            return "";
        }

        // 2. 构造请求参数
        float temperature = 0.7f;
        int max_tokens = 2048;

        // 从 requestParam 提取参数
        if (requestParam.find("temperature") != requestParam.end())
        {
            try
            {
                temperature = std::stof(requestParam.at("temperature"));
            }
            catch (...)
            {
                WARN("Invalid temperature, using default.");
            }
        }
        if (requestParam.find("max_tokens") != requestParam.end())
        {
            try
            {
                max_tokens = std::stoi(requestParam.at("max_tokens"));
            }
            catch (...)
            {
                WARN("Invalid max_tokens, using default.");
            }
        }

        // 3. 构建历史消息 (Messages Array)
        Json::Value messagesArray(Json::arrayValue);
        for (const auto &msg : messages)
        {
            Json::Value messageObj(Json::objectValue);
            messageObj["role"] = msg._role;
            messageObj["content"] = msg._content;
            messagesArray.append(messageObj);
        }

        // 4. 构建请求体 (Request Body)
        Json::Value requestBody(Json::objectValue);
        requestBody["model"] = getModelName(); // gemini-2.5-flash
        requestBody["messages"] = messagesArray;
        requestBody["temperature"] = temperature;
        requestBody["max_tokens"] = max_tokens;
        // 全量模式无需 stream 字段，或显式设为 false

        // 5. 序列化请求体
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = ""; // 紧凑格式
        std::string requestBodyStr = Json::writeString(writerBuilder, requestBody);

        INFO("Gemini Request Body: {}", requestBodyStr);

        // 6. 创建 HTTP 客户端
        // _endpoint 默认为 https://generativelanguage.googleapis.com
        httplib::Client client(_endpoint.c_str());

        // 设置超时时间
        client.set_connection_timeout(30, 0);
        client.set_read_timeout(60, 0);

        // 【重要】Gemini 服务器在海外，必须配置代理
        // 请根据实际环境修改，例如 127.0.0.1:7890
        client.set_proxy("127.0.0.1", 1080);

        // 设置请求头
        httplib::Headers headers = {
            {"Authorization", "Bearer " + _apiKey}
            // Content-Type 会在 Post 方法中指定
        };

        // 7. 发送 POST 请求
        // 路径: /v1beta/openai/chat/completions (OpenAI 兼容接口)
        auto response = client.Post("/v1beta/openai/chat/completions", headers, requestBodyStr, "application/json");

        // 8. 检查响应
        if (!response)
        {
            ERR("GeminiProvider::sendMessage: failed to send request (Network Error): {}", to_string(response.error()));
            return "";
        }

        if (response->status != 200)
        {
            ERR("GeminiProvider::sendMessage: API Error. Status: {}, Body: {}", response->status, response->body);
            return "";
        }

        INFO("Gemini Response Status: {}", response->status);

        // 9. 解析响应体 (反序列化)
        Json::CharReaderBuilder readerBuilder;
        Json::Value responseBody;
        std::string jsonError;
        std::istringstream responseStream(response->body);

        if (!Json::parseFromStream(readerBuilder, responseStream, &responseBody, &jsonError))
        {
            ERR("GeminiProvider::sendMessage: failed to parse response JSON: {}", jsonError);
            return "";
        }

        // 10. 提取回复内容
        // 兼容接口的结构与 OpenAI 一致：choices[0].message.content
        if (responseBody.isMember("choices") &&
            responseBody["choices"].isArray() &&
            !responseBody["choices"].empty())
        {

            Json::Value choice = responseBody["choices"][0];
            if (choice.isMember("message") && choice["message"].isMember("content"))
            {
                std::string reply = choice["message"]["content"].asString();
                INFO("Gemini Reply: {}", reply);
                return reply;
            }
        }

        ERR("GeminiProvider::sendMessage: Invalid response structure");
        return "";
    }

    // 发送消息 - 增量返回 - 流式响应
    std::string GeminiProvider::sendMessageStream(const std::vector<Message> &messages,
                                                  const std::map<std::string, std::string> &requestParam,
                                                  std::function<void(const std::string &, bool)> callback)
    {
        // 1. 检测模型是否可用
        if (!isAvailable())
        {
            ERR("GeminiProvider::sendMessageStream: model not available");
            return "";
        }

        // 2. 构造请求参数
        float temperature = 0.7f;
        int max_tokens = 2048;

        if (requestParam.find("temperature") != requestParam.end())
        {
            temperature = std::stof(requestParam.at("temperature"));
        }
        if (requestParam.find("max_tokens") != requestParam.end())
        {
            max_tokens = std::stoi(requestParam.at("max_tokens"));
        }

        // 构造历史消息
        Json::Value messagesArray(Json::arrayValue);
        for (const auto &msg : messages)
        {
            Json::Value messageObj(Json::objectValue);
            messageObj["role"] = msg._role;
            messageObj["content"] = msg._content;
            messagesArray.append(messageObj);
        }

        // 3. 构造请求体 (开启 stream)
        Json::Value requestBody(Json::objectValue);
        requestBody["model"] = getModelName();
        requestBody["messages"] = messagesArray;
        requestBody["temperature"] = temperature;
        requestBody["max_tokens"] = max_tokens;
        requestBody["stream"] = true; // 关键！

        // 序列化
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "";
        std::string requestBodyStr = Json::writeString(writerBuilder, requestBody);

        // 4. 创建 HTTP 客户端
        httplib::Client client(_endpoint.c_str());
        client.set_connection_timeout(60, 0);
        client.set_read_timeout(300, 0); // 流式响应超时设长
        client.set_proxy("127.0.0.1", 1080); // 科学上网代理

        httplib::Headers headers = {
            {"Authorization", "Bearer " + _apiKey},
            {"Content-Type", "application/json"}};

        // 5. 定义流式处理变量
        std::string buffer;    // 缓冲区
        bool gotError = false; // 错误标记
        std::string errorMsg;
        int statusCode = 0;
        bool streamFinish = false; // 结束标记
        std::string fullData;      // 完整回复累积

        // 6. 构造 Request 对象
        httplib::Request request;
        request.method = "POST";
        request.path = "/v1beta/openai/chat/completions";
        request.body = requestBodyStr;
        request.headers = headers;

        // 7. 响应头处理器
        request.response_handler = [&](const httplib::Response &res)
        {
            statusCode = res.status;
            if (statusCode != 200)
            {
                gotError = true;
                errorMsg = "HTTP status code: " + std::to_string(statusCode);
                ERR("Gemini Stream Handshake Failed: {}", errorMsg);
                return false; // 终止
            }
            return true;
        };

        // 8. 内容接收处理器 (SSE 解析核心)
        request.content_receiver = [&](const char *data, size_t dataLength, uint64_t, uint64_t)
        {
            if (gotError)
                return false;

            // 追加新数据到缓冲区
            buffer.append(data, dataLength);

            // 循环处理缓冲区中的完整消息 (以 \n\n 分隔)
            size_t pos = 0;
            while ((pos = buffer.find("\n\n")) != std::string::npos)
            {
                // 提取一行完整的数据
                std::string line = buffer.substr(0, pos);
                // 移除已处理部分 (+2 跳过 \n\n)
                buffer.erase(0, pos + 2);

                // 忽略空行和 SSE 注释 (以冒号开头)
                if (line.empty() || line[0] == ':')
                    continue;

                // 解析 data: 开头的数据
                if (line.compare(0, 6, "data: ") == 0)
                {
                    std::string dataStr = line.substr(6);

                    // 处理结束标记
                    if (dataStr == "[DONE]")
                    {
                        streamFinish = true;
                        if (callback)
                            callback("", true);
                        return true;
                    }

                    // JSON 反序列化
                    Json::Value chunk;
                    Json::CharReaderBuilder reader;
                    std::stringstream ss(dataStr);
                    std::string errors;

                    if (Json::parseFromStream(reader, ss, &chunk, &errors))
                    {
                        // 提取 content
                        // 路径: choices[0].delta.content
                        if (chunk.isMember("choices") &&
                            chunk["choices"].isArray() &&
                            !chunk["choices"].empty())
                        {

                            auto choice = chunk["choices"][0];
                            if (choice.isMember("delta") &&
                                choice["delta"].isObject() &&
                                choice["delta"].isMember("content"))
                            {

                                std::string deltaContent = choice["delta"]["content"].asString();

                                // 累积完整回复
                                fullData += deltaContent;

                                // 触发回调
                                if (callback)
                                    callback(deltaContent, false);
                            }
                        }
                    }
                    else
                    {
                        ERR("Gemini JSON Parse Failed: {}", errors);
                        // 不终止流，继续处理后续块
                    }
                }
            }
            return true; // 继续接收
        };

        // 9. 发送请求
        auto res = client.send(request);

        // 10. 错误处理
        if (!res)
        {
            ERR("Gemini Network Error: {}", to_string(res.error()));
            return "";
        }

        if (!streamFinish && !gotError)
        {
            WARN("Stream ended without [DONE] marker");
            if (callback)
                callback("", true);
        }

        return fullData;
    }

} // namespace ai_chat_sdk
