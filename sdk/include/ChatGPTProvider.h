#pragma once
#include "LLMProvider.h"
#include <functional>
#include <string>
#include <map>
#include <vector>
#include "common.h"

namespace ai_chat_sdk
{

    /**
     * @brief ChatGPT (OpenAI) 模型接入实现类
     */
    class ChatGPTProvider : public LLMProvider
    {
    public:
        // 初始化模型：解析 API Key 和 Endpoint
        virtual bool initModel(const std::map<std::string, std::string> &modelConfig) override;

        // 检测模型是否有效
        virtual bool isAvailable() const override;

        // 获取模型名称 (如 "gpt-4o-mini")
        virtual std::string getModelName() const override;

        // 获取模型描述
        virtual std::string getModelDesc() const override;

        // 发送消息 - 全量返回
        virtual std::string sendMessage(const std::vector<Message> &messages,
                                        const std::map<std::string, std::string> &requestParam) override;

        // 发送消息 - 增量返回 - 流式响应
        virtual std::string sendMessageStream(const std::vector<Message> &messages,
                                              const std::map<std::string, std::string> &requestParam,
                                              std::function<void(const std::string &, bool)> callback) override;
    };

} // namespace ai_chat_sdk