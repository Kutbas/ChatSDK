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

    // 占位符：核心发送逻辑将在下一章详细实现，涉及 OpenAI 特定的 API 参数
    std::string ChatGPTProvider::sendMessage(const std::vector<Message> &messages,
                                             const std::map<std::string, std::string> &requestParam)
    {
        // TODO: 实现 OpenAI 全量请求
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