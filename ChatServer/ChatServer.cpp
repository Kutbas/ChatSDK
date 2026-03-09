#include "ChatServer.h"
#include <ai_chat_sdk/util/myLog.h>
#include <jsoncpp/json/json.h>

namespace ai_chat_server
{

    ChatServer::ChatServer(const ServerConfig &config) : _config(config)
    {
        // 1. 实例化 ChatSDK
        _chatSDK = std::make_shared<ai_chat_sdk::ChatSDK>();

        // 2. 构造各类模型的配置信息
        // ---------------- DeepSeek ----------------
        auto deepseekConfig = std::make_shared<ai_chat_sdk::APIConfig>();
        deepseekConfig->_modelName = "deepseek-chat";
        deepseekConfig->_apiKey = config.deepseekAPIKey;
        deepseekConfig->_temperature = config.temperature;
        deepseekConfig->_maxTokens = config.maxTokens;

        // ---------------- ChatGPT ----------------
        auto chatGPTConfig = std::make_shared<ai_chat_sdk::APIConfig>();
        chatGPTConfig->_modelName = "gpt-4o-mini";
        chatGPTConfig->_apiKey = config.chatGPTAPIKey;
        chatGPTConfig->_temperature = config.temperature;
        chatGPTConfig->_maxTokens = config.maxTokens;

        // ---------------- Gemini ----------------
        auto geminiConfig = std::make_shared<ai_chat_sdk::APIConfig>();
        geminiConfig->_modelName = "gemini-2.0-flash";
        geminiConfig->_apiKey = config.geminiAPIKey;
        geminiConfig->_temperature = config.temperature;
        geminiConfig->_maxTokens = config.maxTokens;

        // ---------------- Ollama (本地模型) ----------------
        auto ollamaConfig = std::make_shared<ai_chat_sdk::OllamaConfig>();
        ollamaConfig->_modelName = config.ollamaModelName;
        ollamaConfig->_modelDesc = config.ollamaModelDesc;
        ollamaConfig->_endpoint = config.ollamaEndpoint;
        ollamaConfig->_temperature = config.temperature;
        ollamaConfig->_maxTokens = config.maxTokens;

        // 3. 聚合配置并初始化 SDK
        std::vector<std::shared_ptr<ai_chat_sdk::Config>> modelConfigs = {
            deepseekConfig, chatGPTConfig, geminiConfig, ollamaConfig};

        INFO("ChatServer: Start initializing ChatSDK models...");
        if (!_chatSDK->initModels(modelConfigs))
        {
            ERR("ChatServer: ChatSDK init Failed!!!");
            return;
        }
        INFO("ChatServer: ChatSDK models init success!!!");

        // 4. 初始化 HTTP 服务器引擎
        _chatServer = std::make_unique<httplib::Server>();
        if (!_chatServer)
        {
            ERR("ChatServer: HTTP Server instantiation failed!!!");
            return;
        }

        // （预留）在这里调用 setHttpRoutes() 注册路由
    }

} // namespace ai_chat_server