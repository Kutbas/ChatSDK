#include "../include/ChatSDK.h"
#include "../include/DeepSeekProvider.h"
#include "../include/ChatGPTProvider.h"
#include "../include/GeminiProvider.h"
#include "../include/OllamaLLMProvider.h"
#include "../include/util/myLog.h"
#include <memory>
#include <unordered_set>

namespace ai_chat_sdk
{

    // 初始化支持的模型
    bool ChatSDK::initModels(const std::vector<std::shared_ptr<Config>> &configs)
    {
        // 1. 注册所有支持的 Provider
        registerAllProvider(configs);

        // 2. 初始化这些 Provider (注入 Key 和 Endpoint)
        initProviders(configs);

        _initialized = true;
        return true;
    }

    // 注册所有支持的模型
    void ChatSDK::registerAllProvider(const std::vector<std::shared_ptr<Config>> &configs)
    {
        // A. 注册内置的云端模型 Provider
        // ---------------------------------------------------------
        // DeepSeek
        if (!_llmManager.isModelAvailable("deepseek-chat"))
        {
            auto provider = std::make_unique<DeepSeekProvider>();
            // unique_ptr 禁止拷贝，必须使用 std::move 转移所有权
            _llmManager.registerProvider("deepseek-chat", std::move(provider));
            INFO("deepseek-chat provider registered success.");
        }

        // ChatGPT
        if (!_llmManager.isModelAvailable("gpt-4o-mini"))
        {
            auto provider = std::make_unique<ChatGPTProvider>();
            _llmManager.registerProvider("gpt-4o-mini", std::move(provider));
            INFO("gpt-4o-mini provider registered success.");
        }

        // Gemini
        if (!_llmManager.isModelAvailable("gemini-2.5-flash"))
        {
            auto provider = std::make_unique<GeminiProvider>();
            _llmManager.registerProvider("gemini-2.5-flash", std::move(provider));
            INFO("gemini-2.5-flash provider registered success.");
        }

        // B. 注册 Ollama 本地模型 Provider
        // ---------------------------------------------------------
        // 由于用户可能配置了多个 Ollama 模型，我们需要遍历 configs
        std::unordered_set<std::string> registeredOllamaModels;

        for (const auto &config : configs)
        {
            // 使用 RTTI 安全地向下转型
            // 判断当前 config 是否为 OllamaConfig 类型
            auto ollamaConfig = std::dynamic_pointer_cast<OllamaConfig>(config);

            if (ollamaConfig)
            {
                std::string modelName = ollamaConfig->_modelName;

                // 去重检查：防止重复注册同一个模型
                if (registeredOllamaModels.find(modelName) == registeredOllamaModels.end())
                {
                    registeredOllamaModels.insert(modelName);

                    if (!_llmManager.isModelAvailable(modelName))
                    {
                        // 为每一个 Ollama 模型实例一个新的 OllamaLLMProvider
                        _llmManager.registerProvider(modelName, std::make_unique<OllamaLLMProvider>());
                        INFO("Ollama model '{}' provider registered success.", modelName);
                    }
                }
            }
        }
    }

    // 初始化所有支持的模型
    void ChatSDK::initProviders(const std::vector<std::shared_ptr<Config>> &configs)
    {
        for (const auto &config : configs)
        {
            // 尝试转型为 APIConfig (云端模型)
            if (auto apiConfig = std::dynamic_pointer_cast<APIConfig>(config))
            {
                // 校验是否为 SDK 支持的标准云端模型
                if (apiConfig->_modelName == "deepseek-chat" ||
                    apiConfig->_modelName == "gpt-4o-mini" ||
                    apiConfig->_modelName == "gemini-2.5-flash")
                {

                    initAPIModelProviders(apiConfig->_modelName, apiConfig);
                }
                else
                {
                    ERR("Model {} is not supported in API mode", apiConfig->_modelName);
                }
            }
            // 尝试转型为 OllamaConfig (本地模型)
            else if (auto ollamaConfig = std::dynamic_pointer_cast<OllamaConfig>(config))
            {
                initOllamaModelProviders(ollamaConfig->_modelName, ollamaConfig);
            }
            else
            {
                ERR("Config type for {} is not supported", config->_modelName);
            }
        }
    }

    bool ChatSDK::initAPIModelProviders(const std::string &modelName, const std::shared_ptr<APIConfig> &apiConfig)
    {
        // 1. 参数检测
        if (modelName.empty())
        {
            ERR("ChatSDK::initAPIModelProviders: modelName is empty");
            return false;
        }
        if (!apiConfig || apiConfig->_apiKey.empty())
        {
            ERR("ChatSDK::initAPIModelProviders: apiKey is empty for model {}", modelName);
            return false;
        }

        // 2. 幂等性检查：防止重复初始化
        if (_llmManager.isModelAvailable(modelName))
        {
            INFO("ChatSDK::initAPIModelProviders: model {} is already available", modelName);
            return true;
        }

        // 3. 调用底层 Manager 进行初始化
        std::map<std::string, std::string> modelParams;
        modelParams["api_key"] = apiConfig->_apiKey;

        if (!_llmManager.initModel(modelName, modelParams))
        {
            ERR("ChatSDK::initAPIModelProviders: init model {} failed", modelName);
            return false;
        }

        // 4. 缓存配置信息，供后续 sendMessage 使用
        _modelConfigs[modelName] = apiConfig;

        INFO("ChatSDK::initAPIModelProviders: model {} init success", modelName);
        return true;
    }

    bool ChatSDK::initOllamaModelProviders(const std::string &modelName, const std::shared_ptr<OllamaConfig> &ollamaConfig)
    {
        // 1. 参数检测
        if (modelName.empty())
        {
            ERR("ChatSDK::initOllamaModelProviders: modelName is empty");
            return false;
        }
        if (!ollamaConfig || ollamaConfig->_endpoint.empty())
        {
            ERR("ChatSDK::initOllamaModelProviders: endpoint is empty for model {}", modelName);
            return false;
        }

        // 2. 幂等性检查
        if (_llmManager.isModelAvailable(modelName))
        {
            INFO("ChatSDK::initOllamaModelProviders: model {} is already available", modelName);
            return true;
        }

        // 3. 构建初始化参数包
        // OllamaProvider 需要这些信息来定位本地服务和具体的模型文件
        std::map<std::string, std::string> modelParams;
        modelParams["model_name"] = modelName;
        modelParams["model_desc"] = ollamaConfig->_modelDesc;
        modelParams["endpoint"] = ollamaConfig->_endpoint;

        if (!_llmManager.initModel(modelName, modelParams))
        {
            ERR("ChatSDK::initOllamaModelProviders: init model {} failed", modelName);
            return false;
        }

        // 4. 缓存配置
        _modelConfigs[modelName] = ollamaConfig;

        INFO("ChatSDK::initOllamaModelProviders: model {} init success", modelName);
        return true;
    }

    // 创建会话
    // 用户指定要聊天的模型名称（如 "deepseek-chat"），SDK 返回生成的 Session ID
    std::string ChatSDK::createSession(const std::string &modelName)
    {
        // 1. 安全检查：SDK 必须先初始化
        if (!_initialized)
        {
            ERR("ChatSDK::createSession: SDK is not initialized");
            return "";
        }

        // 2. 转调 SessionManager 创建会话
        // 具体的 ID 生成、数据库持久化逻辑都在 SessionManager 内部处理
        auto sessionId = _sessionManager.createSession(modelName);

        if (sessionId.empty())
        {
            ERR("ChatSDK::createSession: create session failed for model {}", modelName);
            return "";
        }

        INFO("ChatSDK::createSession: create session {} success", sessionId);
        return sessionId;
    }

    // 获取指定会话对象
    std::shared_ptr<Session> ChatSDK::getSession(const std::string &sessionId)
    {
        if (!_initialized)
        {
            ERR("ChatSDK::getSession: SDK is not initialized");
            return nullptr;
        }

        // 从 SessionManager 获取（包含了一级内存缓存和二级数据库查找）
        auto session = _sessionManager.getSession(sessionId);
        if (!session)
        {
            ERR("ChatSDK::getSession: session {} not found", sessionId);
            return nullptr;
        }
        return session;
    }

    // 获取所有会话列表
    // 返回的是 Session ID 的列表，通常用于前端侧边栏展示
    std::vector<std::string> ChatSDK::getSessionList() const
    {
        if (!_initialized)
        {
            ERR("ChatSDK::getSessionList: SDK is not initialized");
            return {};
        }

        // SessionManager 内部已经实现了按时间戳降序排列
        return _sessionManager.getSessionList();
    }

    // 删除会话
    bool ChatSDK::deleteSession(const std::string &sessionId)
    {
        if (!_initialized)
        {
            ERR("ChatSDK::deleteSession: SDK is not initialized");
            return false;
        }

        // 删除会话会触发级联删除，清理该会话下的所有历史消息
        bool result = _sessionManager.deleteSession(sessionId);
        if (!result)
        {
            ERR("ChatSDK::deleteSession: delete session {} failed", sessionId);
            return false;
        }

        INFO("ChatSDK::deleteSession: delete session {} success", sessionId);
        return true;
    }

    // ================= 模型信息接口 =================

    // 获取当前可用的模型列表
    // 用于 UI 层展示“模型选择器”
    std::vector<ModelInfo> ChatSDK::getAvailableModels() const
    {
        // 直接转调 LLMManager，它维护了所有已注册且初始化成功的模型
        return _llmManager.getAvailableModels();
    }

    // 给模型发消息 - 全量返回
    std::string ChatSDK::sendMessage(const std::string &sessionId, const std::string &message)
    {
        // 1. 检测SDK是否初始化成功
        if (!_initialized)
        {
            ERR("ChatSDK::sendMessage: SDK is not initialized");
            return "";
        }

        // 2. 获取sessionId对应的session对象
        auto session = _sessionManager.getSession(sessionId);
        if (!session)
        {
            ERR("ChatSDK::sendMessage: session {} not found", sessionId);
            return "";
        }

        // 3. 构造并保存用户消息 (User Message)
        // 必须先保存，这样 getHistoryMessages 才能包含当前这条问题
        Message userMessage("user", message);
        _sessionManager.addMessage(sessionId, userMessage);

        // 4. 获取完整的历史上下文
        auto historyMessages = _sessionManager.getHistoryMessages(sessionId);

        // 5. 构建请求参数 (从缓存的配置中读取)
        auto it = _modelConfigs.find(session->_modelName);
        if (it == _modelConfigs.end())
        {
            ERR("ChatSDK::sendMessage: config for model {} not found", session->_modelName);
            return "";
        }

        std::map<std::string, std::string> requestParam;
        requestParam["temperature"] = std::to_string(it->second->_temperature);
        requestParam["max_tokens"] = std::to_string(it->second->_maxTokens);

        // 6. 调用 LLMManager 发送消息
        auto response = _llmManager.sendMessage(session->_modelName, historyMessages, requestParam);

        if (response.empty())
        {
            ERR("ChatSDK::sendMessage: send message to model {} failed", session->_modelName);
            return "";
        }

        // 7. 保存助手消息 (Assistant Message) 并更新会话时间
        Message assistantMessage("assistant", response);
        _sessionManager.addMessage(sessionId, assistantMessage);
        _sessionManager.updateSessionTimestamp(sessionId); // 确保该会话排到列表最前

        INFO("ChatSDK::sendMessage: send message to model {} success", session->_modelName);
        return response;
    }

    // 给模型发送消息 - 增量返回
    std::string ChatSDK::sendMessageStream(const std::string &sessionId,
                                           const std::string &message,
                                           std::function<void(const std::string &, bool)> callback)
    {
        // 1. 检测SDK是否初始化
        if (!_initialized)
        {
            ERR("ChatSDK::sendMessageStream: SDK is not initialized");
            return "";
        }

        // 2. 获取 Session
        auto session = _sessionManager.getSession(sessionId);
        if (!session)
        {
            ERR("ChatSDK::sendMessageStream: session {} not found", sessionId);
            return "";
        }

        // 3. 保存用户提问
        Message userMessage("user", message);
        _sessionManager.addMessage(sessionId, userMessage);

        // 4. 获取历史上下文
        auto historyMessages = _sessionManager.getHistoryMessages(sessionId);

        // 5. 获取配置参数
        auto it = _modelConfigs.find(session->_modelName);
        if (it == _modelConfigs.end())
        {
            ERR("ChatSDK::sendMessageStream: model {} not found", session->_modelName);
            return "";
        }

        std::map<std::string, std::string> requestParam;
        requestParam["temperature"] = std::to_string(it->second->_temperature);
        requestParam["max_tokens"] = std::to_string(it->second->_maxTokens);

        // 6. 调用 LLMManager 发送流式请求
        // 注意：底层的 sendMessageStream 是阻塞的，它会不断触发 callback，
        // 直到流结束，并返回完整的拼接字符串 response。
        auto response = _llmManager.sendMessageStream(session->_modelName, historyMessages, requestParam, callback);

        if (response.empty())
        {
            ERR("ChatSDK::sendMessageStream: send message to model {} failed", session->_modelName);
            return "";
        }

        // 7. 保存完整的助手回复 (持久化)
        // 尽管用户已经在 callback 中看到了内容，但在数据库中我们需要存一份完整的
        Message assistantMessage("assistant", response);
        _sessionManager.addMessage(sessionId, assistantMessage);
        _sessionManager.updateSessionTimestamp(sessionId);

        INFO("ChatSDK::sendMessageStream: send message to model {} success", session->_modelName);
        return response;
    }
} // namespace ai_chat_sdk