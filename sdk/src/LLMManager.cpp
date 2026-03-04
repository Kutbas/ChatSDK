#include "../include/LLMManager.h"
#include "../include/util/myLog.h"
#include "../include/common.h"

namespace ai_chat_sdk
{

    // 注册LLM提供者
    bool LLMManager::registerProvider(const std::string &modelName, std::unique_ptr<LLMProvider> provider)
    {
        // 1. 参数检测
        if (!provider)
        {
            ERR("LLMManager::registerProvider: cannot register nullptr provider, modelName = {}", modelName);
            return false;
        }

        // 2. 转移资源所有权 (Move Semantics)
        // unique_ptr 禁止拷贝，只能移动
        _providers[modelName] = std::move(provider);

        // 3. 初始化元数据缓存
        // 刚注册时，模型尚未初始化，因此 isAvailable 默认为 false
        _modelInfos[modelName] = ModelInfo(modelName);

        INFO("LLMManager: register provider success, modelName = {}", modelName);
        return true;
    }

    // 初始化指定模型
    bool LLMManager::initModel(const std::string &modelName, const std::map<std::string, std::string> &modelParam)
    {
        // 1. 查找模型是否已注册
        auto it = _providers.find(modelName);
        if (it == _providers.end())
        {
            ERR("LLMManager::initModel: model provider not found, modelName = {}", modelName);
            return false;
        }

        // 2. 调用具体 Provider 的初始化逻辑 (多态调用)
        // it->second 是 unique_ptr<LLMProvider>
        bool isSuccess = it->second->initModel(modelParam);

        // 3. 更新元数据状态
        if (!isSuccess)
        {
            ERR("LLMManager::initModel: init failed, modelName = {}", modelName);
        }
        else
        {
            INFO("LLMManager::initModel: init success, modelName = {}", modelName);
            // 更新缓存的描述信息和可用状态
            _modelInfos[modelName]._modelDesc = it->second->getModelDesc();
            _modelInfos[modelName]._isAvailable = true;
        }

        return isSuccess;
    }

    // 获取可用模型
    std::vector<ModelInfo> LLMManager::getAvailableModels() const
    {
        std::vector<ModelInfo> models;
        for (const auto &pair : _modelInfos)
        {
            // 只返回初始化成功的模型
            if (pair.second._isAvailable)
            {
                models.push_back(pair.second);
            }
        }
        return models;
    }

    // 检查模型是否可用
    bool LLMManager::isModelAvailable(const std::string &modelName) const
    {
        auto it = _modelInfos.find(modelName);
        if (it == _modelInfos.end())
        {
            return false;
        }
        return it->second._isAvailable;
    }

    // 发送消息给指定模型
    std::string LLMManager::sendMessage(const std::string &modelName,
                                        const std::vector<Message> &messages,
                                        const std::map<std::string, std::string> &requestParam)
    {
        // 1. 查找 Provider
        auto it = _providers.find(modelName);
        if (it == _providers.end())
        {
            ERR("LLMManager::sendMessage: model provider not found, modelName = {}", modelName);
            return "";
        }

        // 2. 双重检测可用性
        // 虽然 _modelInfos 中有状态，但直接检查 Provider 实例更保险
        if (!it->second->isAvailable())
        {
            ERR("LLMManager::sendMessage: model not available (not initialized), modelName = {}", modelName);
            return "";
        }

        // 3. 转发调用
        return it->second->sendMessage(messages, requestParam);
    }

    // 发送消息流给指定模型
    std::string LLMManager::sendMessageStream(const std::string &modelName,
                                              const std::vector<Message> &messages,
                                              const std::map<std::string, std::string> &requestParam,
                                              std::function<void(const std::string &, bool)> callback)
    {
        // 逻辑同上
        auto it = _providers.find(modelName);
        if (it == _providers.end())
        {
            ERR("LLMManager::sendMessageStream: provider not found, modelName = {}", modelName);
            return "";
        }

        if (!it->second->isAvailable())
        {
            ERR("LLMManager::sendMessageStream: model not available, modelName = {}", modelName);
            return "";
        }

        return it->second->sendMessageStream(messages, requestParam, callback);
    }

} // end ai_chat_sdk
