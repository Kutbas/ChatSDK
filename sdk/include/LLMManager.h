#pragma once
#include <map>
#include <memory>
#include "LLMProvider.h"

namespace ai_chat_sdk
{

    class LLMManager
    {
    public:
        // 注册 LLM 提供者 (依赖注入)
        // modelName: 如 "deepseek-chat"
        // provider: 具体的 Provider 实例 (如 new DeepSeekProvider)
        bool registerProvider(const std::string &modelName, std::unique_ptr<LLMProvider> provider);

        // 初始化指定模型
        bool initModel(const std::string &modelName, const std::map<std::string, std::string> &modelParam);

        // 获取当前所有已注册且可用的模型列表
        std::vector<ModelInfo> getAvailableModels() const;

        // 检查指定模型是否可用
        bool isModelAvailable(const std::string &modelName) const;

        // 核心接口：发送消息给指定模型 (全量)
        std::string sendMessage(const std::string &modelName,
                                const std::vector<Message> &messages,
                                const std::map<std::string, std::string> &requestParam);

        // 核心接口：发送消息给指定模型 (流式)
        std::string sendMessageStream(const std::string &modelName,
                                      const std::vector<Message> &messages,
                                      const std::map<std::string, std::string> &requestParam,
                                      std::function<void(const std::string &, bool)> callback);

    private:
        // Key: 模型名称, Value: 模型 Provider 实例 (多态)
        std::map<std::string, std::unique_ptr<LLMProvider>> _providers;

        // Key: 模型名称, Value: 模型元信息 (缓存)
        std::map<std::string, ModelInfo> _modelInfos;
    };

} // namespace ai_chat_sdk