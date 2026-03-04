#pragma once
#include "LLMProvider.h"

namespace ai_chat_sdk
{

    /**
     * @brief Ollama 本地模型接入实现类
     */
    class OllamaLLMProvider : public LLMProvider
    {
    public:
        // 初始化模型
        // modelConfig 需包含: "model_name", "endpoint", 可选 "model_desc"
        virtual bool initModel(const std::map<std::string, std::string> &modelConfig) override;

        // 检测模型是否有效
        virtual bool isAvailable() const override;

        // 获取模型名称 (动态配置)
        virtual std::string getModelName() const override;

        // 获取模型描述 (动态配置)
        virtual std::string getModelDesc() const override;

        // 发送消息 - 全量返回
        virtual std::string sendMessage(const std::vector<Message> &messages,
                                        const std::map<std::string, std::string> &requestParam) override;

        // 发送消息 - 增量返回 - 流式响应
        virtual std::string sendMessageStream(const std::vector<Message> &messages,
                                              const std::map<std::string, std::string> &requestParam,
                                              std::function<void(const std::string &, bool)> callback) override;

    protected:
        std::string _modelName; // 模型名称 (如 "deepseek-r1:1.5b")
        std::string _modelDesc; // 模型描述
    };

} // namespace ai_chat_sdk