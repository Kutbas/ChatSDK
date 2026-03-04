#include "../include/OllamaLLMProvider.h"
#include "../include/util/myLog.h"
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <httplib.h>
#include <sstream>

namespace ai_chat_sdk
{

    bool OllamaLLMProvider::initModel(const std::map<std::string, std::string> &modelConfig)
    {
        // 1. 初始化模型名称 (必填)
        // 因为 Ollama 可以运行各种模型，我们需要知道具体调用哪一个
        if (modelConfig.find("model_name") == modelConfig.end())
        {
            ERR("OllamaLLMProvider::initModel: 'model_name' not found in config.");
            return false;
        }
        _modelName = modelConfig.at("model_name");

        // 2. 初始化模型描述 (选填)
        if (modelConfig.find("model_desc") != modelConfig.end())
        {
            _modelDesc = modelConfig.at("model_desc");
        }
        else
        {
            _modelDesc = "Local Ollama Model: " + _modelName;
        }

        // 3. 初始化 Endpoint (必填)
        // 通常为 http://127.0.0.1:11434
        if (modelConfig.find("endpoint") == modelConfig.end())
        {
            ERR("OllamaLLMProvider::initModel: 'endpoint' not found in config.");
            return false;
        }
        _endpoint = modelConfig.at("endpoint");

        // 4. 标记初始化成功
        _isAvailable = true;
        INFO("OllamaLLMProvider init success. Model: {}, Endpoint: {}", _modelName, _endpoint);
        return true;
    }

    bool OllamaLLMProvider::isAvailable() const
    {
        return _isAvailable;
    }

    std::string OllamaLLMProvider::getModelName() const
    {
        return _modelName;
    }

    std::string OllamaLLMProvider::getModelDesc() const
    {
        return _modelDesc;
    }

    // 发送消息 - 全量返回
    std::string OllamaLLMProvider::sendMessage(const std::vector<Message> &messages,
                                               const std::map<std::string, std::string> &requestParam)
    {
        // 1. 检测模型是否可用
        if (!isAvailable())
        {
            ERR("OllamaLLMProvider::sendMessage: model is not available");
            return "";
        }

        // 2. 构造请求参数
        // 默认值
        float temperature = 0.7f;
        int maxTokens = 2048;

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
                maxTokens = std::stoi(requestParam.at("max_tokens"));
            }
            catch (...)
            {
                WARN("Invalid max_tokens, using default.");
            }
        }

        // 3. 构建历史消息 (Messages Array)
        Json::Value messageArray(Json::arrayValue);
        for (const auto &msg : messages)
        {
            Json::Value messageObject(Json::objectValue);
            messageObject["role"] = msg._role;
            messageObject["content"] = msg._content;
            messageArray.append(messageObject);
        }

        // 4. 构建请求体 (Request Body)
        // 注意：Ollama 将高级参数放在 "options" 对象中
        Json::Value options(Json::objectValue);
        options["temperature"] = temperature;
        options["num_ctx"] = maxTokens; // Ollama 使用 num_ctx 而非 max_tokens

        Json::Value requestBody(Json::objectValue);
        requestBody["model"] = _modelName; // 如 "deepseek-r1:1.5b"
        requestBody["messages"] = messageArray;
        requestBody["options"] = options;
        requestBody["stream"] = false; // 全量返回模式

        // 5. 序列化请求体
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = ""; // 紧凑格式
        std::string requestBodyStr = Json::writeString(writerBuilder, requestBody);

        INFO("Ollama Request Body: {}", requestBodyStr);

        // 6. 创建 HTTP 客户端
        // _endpoint 通常为 http://127.0.0.1:11434
        httplib::Client client(_endpoint.c_str());

        // 设置超时时间
        client.set_connection_timeout(30, 0); // 连接超时 30秒
        client.set_read_timeout(60, 0);       // 读取超时 60秒 (本地推理可能较慢)

        // 设置请求头
        httplib::Headers headers = {
            {"Content-Type", "application/json"}};

        // 7. 发送 POST 请求
        // 路径为 /api/chat
        auto response = client.Post("/api/chat", headers, requestBodyStr, "application/json");

        // 8. 检查响应
        if (!response)
        {
            ERR("OllamaLLMProvider::sendMessage: failed to send request (Network Error): {}", to_string(response.error()));
            return "";
        }

        if (response->status != 200)
        {
            ERR("OllamaLLMProvider::sendMessage: API Error. Status: {}, Body: {}", response->status, response->body);
            return "";
        }

        INFO("Ollama Response Status: {}", response->status);

        // 9. 解析响应体 (反序列化)
        Json::CharReaderBuilder readerBuilder;
        Json::Value responseBody;
        std::string errors;
        std::istringstream responseStream(response->body);

        if (!Json::parseFromStream(readerBuilder, responseStream, &responseBody, &errors))
        {
            ERR("OllamaLLMProvider::sendMessage: failed to parse response JSON: {}", errors);
            return "";
        }

        // 10. 提取回复内容
        // 路径: message -> content
        if (responseBody.isMember("message") &&
            responseBody["message"].isObject() &&
            responseBody["message"].isMember("content"))
        {

            std::string modelResponse = responseBody["message"]["content"].asString();
            INFO("Ollama Reply: {}", modelResponse);
            return modelResponse;
        }

        ERR("OllamaLLMProvider::sendMessage: Invalid response format");
        return "";
    }

    std::string OllamaLLMProvider::sendMessageStream(const std::vector<Message> &messages,
                                                     const std::map<std::string, std::string> &requestParam,
                                                     std::function<void(const std::string &, bool)> callback)
    {
        // TODO: 实现 Ollama 流式请求
        return "";
    }

} // namespace ai_chat_sdk
