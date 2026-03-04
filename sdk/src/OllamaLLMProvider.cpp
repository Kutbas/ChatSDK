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

    // 占位符：核心发送逻辑将在下一章详细实现
    std::string OllamaLLMProvider::sendMessage(const std::vector<Message> &messages,
                                               const std::map<std::string, std::string> &requestParam)
    {
        // TODO: 实现 Ollama 全量请求
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
