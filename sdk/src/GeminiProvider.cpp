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

    std::string GeminiProvider::sendMessageStream(const std::vector<Message> &messages,
                                                  const std::map<std::string, std::string> &requestParam,
                                                  std::function<void(const std::string &, bool)> callback)
    {
        // TODO: 实现 Gemini 流式请求
        return "";
    }

} // namespace ai_chat_sdk
