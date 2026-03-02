#include "../include/ChatGPTProvider.h"
#include "../include/util/myLog.h"
#include <jsoncpp/json/json.h>
#include <sstream>
#include <httplib.h>

namespace ai_chat_sdk
{

    bool ChatGPTProvider::initModel(const std::map<std::string, std::string> &modelConfig)
    {
        // 1. 提取 API Key
        auto it = modelConfig.find("api_key");
        if (it == modelConfig.end())
        {
            ERR("ChatGPTProvider initModel: 'api_key' not found in config.");
            return false;
        }
        _apiKey = it->second;

        // 2. 提取 Endpoint (默认使用 OpenAI 官方地址)
        it = modelConfig.find("endpoint");
        if (it == modelConfig.end())
        {
            _endpoint = "https://api.openai.com";
        }
        else
        {
            _endpoint = it->second;
        }

        _isAvailable = true;
        INFO("ChatGPTProvider init success. Endpoint: {}", _endpoint);
        return true;
    }

    bool ChatGPTProvider::isAvailable() const
    {
        return _isAvailable;
    }

    std::string ChatGPTProvider::getModelName() const
    {
        // 这里硬编码为 gpt-4o-mini，也可以在 initModel 中动态配置
        return "gpt-4o-mini";
    }

    std::string ChatGPTProvider::getModelDesc() const
    {
        return "OpenAI GPT-4o-mini: OpenAI 推出的轻量级、高性价比模型，核心能力接近 GPT-4 Turbo，但更经济。";
    }

    // 发送消息 - 全量返回
    std::string ChatGPTProvider::sendMessage(const std::vector<Message> &messages,
                                             const std::map<std::string, std::string> &requestParam)
    {
        // 1. 检测模型是否可用
        if (!isAvailable())
        {
            ERR("ChatGPTProvider sendMessage: Model is not available.");
            return "";
        }

        // 2. 构造请求参数
        double temperature = 0.7;
        int maxOutputTokens = 2048; // OpenAI 新版 API 参数名可能调整为 max_output_tokens

        // 从 requestParam 提取参数
        if (requestParam.find("temperature") != requestParam.end())
        {
            try
            {
                temperature = std::stod(requestParam.at("temperature"));
            }
            catch (...)
            {
                WARN("Invalid temperature, using default.");
            }
        }
        if (requestParam.find("max_output_tokens") != requestParam.end())
        {
            try
            {
                maxOutputTokens = std::stoi(requestParam.at("max_output_tokens"));
            }
            catch (...)
            {
                WARN("Invalid max_output_tokens, using default.");
            }
        }

        // 3. 构建消息列表 (Input)
        // 根据 Responses API 风格或 Chat Completions API 风格调整
        // 这里我们演示适配 Chat Completions API 的标准格式
        Json::Value messagesArray(Json::arrayValue);
        for (const auto &msg : messages)
        {
            Json::Value messageJson(Json::objectValue);
            messageJson["role"] = msg._role;
            messageJson["content"] = msg._content;
            messagesArray.append(messageJson);
        }

        // 4. 构造请求体 (Body)
        Json::Value requestBody;
        requestBody["model"] = getModelName(); // gpt-4o-mini
        requestBody["input"] = messagesArray;  // 注意：Chat Responses API 使用 "input"
        requestBody["temperature"] = temperature;
        requestBody["max_output_tokens"] = maxOutputTokens; // Chat Responses API 使用 "max_output_tokens"

        // 5. 序列化
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "";
        std::string requestBodyStr = Json::writeString(writerBuilder, requestBody);

        INFO("ChatGPT Request Body: {}", requestBodyStr);

        // 6. 创建 HTTP 客户端
        httplib::Client client(_endpoint.c_str());
        client.set_connection_timeout(30, 0);
        client.set_read_timeout(60, 0);

        // 【关键】设置代理 (科学上网必须)
        // 请根据你的实际代理端口修改，例如 127.0.0.1:7890
        client.set_proxy("127.0.0.1", 1080);

        // 设置请求头
        httplib::Headers headers = {
            {"Authorization", "Bearer " + _apiKey}};

        // 7. 发送 POST 请求
        // 路径: /v1/responses
        auto response = client.Post("/v1/responses", headers, requestBodyStr, "application/json");

        // 8. 检查响应
        if (!response)
        {
            ERR("ChatGPT Network Error: {}", to_string(response.error()));
            return "";
        }
        if (response->status != 200)
        {
            ERR("ChatGPT API Error. Status: {}, Body: {}", response->status, response->body);
            return "";
        }

        // 9. 解析响应体
        Json::Value responseJson;
        Json::CharReaderBuilder reader;
        std::string errorJson;
        std::istringstream responseStream(response->body);

        if (!Json::parseFromStream(reader, responseStream, &responseJson, &errorJson))
        {
            ERR("ChatGPT JSON Parse Failed: {}", errorJson);
            return "";
        }

        // 10. 提取回复内容
        if (responseJson.isMember("output") && responseJson["output"].isArray() && !responseJson["output"].empty())
        {
            // 模型的回复刚好是output数组的第0个元素
            auto output = responseJson["output"][0];
            if (output.isMember("content") && output["content"].isArray() && !output["content"].empty() && output["content"][0].isMember("text"))
            {
                std::string replyString = output["content"][0]["text"].asString();
                INFO("ChatGPTProvider sendMessage replyString: {}", replyString);
                return replyString;
            }
        }

        ERR("ChatGPT Invalid Response Structure");
        return "";
    }

    std::string ChatGPTProvider::sendMessageStream(const std::vector<Message> &messages,
                                                   const std::map<std::string, std::string> &requestParam,
                                                   std::function<void(const std::string &, bool)> callback)
    {
        // 1. 检测模型是否可用
        if (!isAvailable())
        {
            ERR("ChatGPTProvider sendMessageStream: Model not available.");
            return "";
        }

        // 2. 构造请求参数
        double temperature = 0.7;
        int maxOutputTokens = 2048;

        if (requestParam.find("temperature") != requestParam.end())
        {
            temperature = std::stod(requestParam.at("temperature"));
        }
        if (requestParam.find("max_output_tokens") != requestParam.end())
        {
            maxOutputTokens = std::stoi(requestParam.at("max_output_tokens"));
        }

        // 构建消息列表
        Json::Value messagesArray(Json::arrayValue);
        for (const auto &msg : messages)
        {
            Json::Value messageJson(Json::objectValue);
            messageJson["role"] = msg._role;
            messageJson["content"] = msg._content;
            messagesArray.append(messageJson);
        }

        // 构建请求体 (注意 stream = true)
        Json::Value requestBody;
        requestBody["model"] = getModelName();
        requestBody["input"] = messagesArray; // Responses API 可能使用 input，Chat API 使用 messages，需根据实际 Endpoint 调整
        // 这里为了演示 Responses API 的流式特性，假设我们使用 /v1/responses
        // 如果使用 /v1/chat/completions，请改回 messages 并调整解析逻辑
        requestBody["temperature"] = temperature;
        requestBody["max_output_tokens"] = maxOutputTokens;
        requestBody["stream"] = true;

        // 3. 序列化
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "";
        std::string requestBodyStr = Json::writeString(writerBuilder, requestBody);

        // 4. 创建 HTTP 客户端
        httplib::Client client(_endpoint.c_str());
        client.set_connection_timeout(60, 0);
        client.set_read_timeout(300, 0);     // 流式响应超时设长
        client.set_proxy("127.0.0.1", 1080); // 代理设置

        httplib::Headers headers = {
            {"Authorization", "Bearer " + _apiKey},
            {"Content-Type", "application/json"},
            {"Accept", "text/event-stream"} // 显式声明接收 SSE
        };

        // 5. 定义流式处理上下文
        std::string buffer;
        bool gotError = false;
        std::string errorMsg;
        bool streamFinish = false;
        std::string fullResponse;

        // 6. 构造 Request 对象
        httplib::Request req;
        req.method = "POST";
        req.path = "/v1/responses"; // 注意：这里假设使用新版 Responses API
        req.body = requestBodyStr;
        req.headers = headers;

        // 7. 响应头处理器
        req.response_handler = [&](const httplib::Response &res)
        {
            if (res.status != 200)
            {
                gotError = true;
                errorMsg = "HTTP Status Error: " + std::to_string(res.status);
                ERR("{}", errorMsg);
                return false; // 终止连接
            }
            return true;
        };

        // 8. 内容接收处理器 (SSE 解析核心)
        req.content_receiver = [&](const char *data, size_t len, uint64_t /*offset*/, uint64_t /*total*/)
        {
            if (gotError)
                return false;

            buffer.append(data, len);
            INFO("Received Chunk Size: {}", len);

            size_t pos = 0;
            // 循环处理 buffer 中的完整消息 (以 \n\n 分隔)
            while ((pos = buffer.find("\n\n")) != std::string::npos)
            {
                std::string eventBlock = buffer.substr(0, pos);
                buffer.erase(0, pos + 2); // 移除已处理部分

                // 解析 event 和 data 字段
                std::istringstream ss(eventBlock);
                std::string line;
                std::string eventType;
                std::string eventData;

                while (std::getline(ss, line))
                {
                    if (line.empty())
                        continue;
                    if (line.compare(0, 6, "event:") == 0)
                    {
                        // 提取 event 类型 (去除前缀 event: )
                        eventType = line.substr(6);
                        // 去除可能的前导空格
                        if (!eventType.empty() && eventType[0] == ' ')
                            eventType.erase(0, 1);
                    }
                    else if (line.compare(0, 5, "data:") == 0)
                    {
                        // 提取 json 数据
                        eventData = line.substr(5);
                    }
                }

                INFO("Event: {}, Data Length: {}", eventType, eventData.length());

                // 反序列化 JSON 数据
                Json::Value chunkJson;
                Json::CharReaderBuilder reader;
                std::string errs;
                std::istringstream dataStream(eventData);
                if (!Json::parseFromStream(reader, dataStream, &chunkJson, &errs))
                {
                    WARN("JSON Parse Failed: {}", errs);
                    continue;
                }

                // 根据事件类型分发逻辑
                if (eventType == "response.output_text.delta")
                {
                    // 处理文本增量
                    if (chunkJson.isMember("delta") && chunkJson["delta"].isString())
                    {
                        std::string delta = chunkJson["delta"].asString();
                        if (callback)
                            callback(delta, false);
                    }
                }
                else if (eventType == "response.output_item.done")
                {
                    // 处理 Item 结束，累积完整回复
                    if (chunkJson.isMember("item") && chunkJson["item"].isObject())
                    {
                        auto item = chunkJson["item"];
                        if (item.isMember("content") && item["content"].isArray() && !item["content"].empty())
                        {
                            auto contentObj = item["content"][0];
                            if (contentObj.isMember("text") && contentObj["text"].isString())
                            {
                                fullResponse += contentObj["text"].asString();
                            }
                        }
                    }
                }
                else if (eventType == "response.completed")
                {
                    // 流结束
                    streamFinish = true;
                    if (callback)
                        callback("", true);
                    return true; // 也可以返回 false 主动断开
                }
            }
            return true;
        };

        // 9. 发送请求
        auto res = client.send(req);

        // 10. 错误处理
        if (!res)
        {
            ERR("Network Error: {}", to_string(res.error()));
            return "";
        }

        if (!streamFinish && !gotError)
        {
            WARN("Stream ended unexpectedly without response.completed");
            if (callback)
                callback("", true);
        }

        return fullResponse;
    }

} // namespace ai_chat_sdk