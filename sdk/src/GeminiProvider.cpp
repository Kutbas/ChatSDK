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
        return "gemini-2.0-flash";
    }

    std::string GeminiProvider::getModelDesc() const
    {
        return "Google Gemini 2.0 Flash: Google 的极速响应模型，专为低延迟、高吞吐量的交互场景设计。";
    }

    // 占位符：核心发送逻辑将在后续章节详细实现
    std::string GeminiProvider::sendMessage(const std::vector<Message> &messages,
                                            const std::map<std::string, std::string> &requestParam)
    {
        // TODO: 实现 Gemini 全量请求
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
