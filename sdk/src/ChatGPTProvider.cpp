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
        client.set_proxy("127.0.0.1", 7890);

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
        // TODO: 实现 OpenAI 流式请求
        return "";
    }

} // namespace ai_chat_sdk