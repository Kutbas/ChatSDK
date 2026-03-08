#pragma once
#include <memory>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include "LLMManager.h"
#include "SessionManager.h"
#include "common.h"

namespace ai_chat_sdk
{
    // ChatSDK 类
    class ChatSDK
    {
    public:
        /**
         * @brief SDK 初始化入口
         * @param configs 配置列表，包含所有需要接入的模型的配置信息（如 API Key, Endpoint, 温度等）
         * @return true 初始化成功, false 失败
         */
        bool initModels(const std::vector<std::shared_ptr<Config>> &configs);

        /**
         * @brief 创建新会话
         * @param modelName 指定该会话使用的模型名称 (如 "deepseek-chat")
         * @return 生成的唯一会话 ID
         */
        std::string createSession(const std::string &modelName);

        /**
         * @brief 获取指定会话对象
         * @param sessionId 会话 ID
         */
        std::shared_ptr<Session> getSession(const std::string &sessionId);

        /**
         * @brief 获取所有历史会话 ID 列表
         * 通常用于前端侧边栏展示
         */
        std::vector<std::string> getSessionList() const;

        /**
         * @brief 删除指定会话
         */
        bool deleteSession(const std::string &sessionId);

        /**
         * @brief 获取当前所有可用模型的信息
         * @return 模型信息列表 (名称, 描述, 状态)
         */
        std::vector<ModelInfo> getAvailableModels() const;

        /**
         * @brief 发送消息 (全量返回)
         * @param sessionId 会话 ID
         * @param message 用户输入的文本
         * @return AI 回复的完整文本
         */
        std::string sendMessage(const std::string &sessionId, const std::string &message);

        /**
         * @brief 发送消息 (流式响应)
         * @param sessionId 会话 ID
         * @param message 用户输入的文本
         * @param callback 回调函数，用于接收增量数据
         *                 func(chunk, is_done)
         * @return 拼接后的完整回复
         */
        std::string sendMessageStream(const std::string &sessionId,
                                      const std::string &message,
                                      std::function<void(const std::string &, bool)> callback);

    private:
        // 内部辅助：注册并初始化所有 Provider
        void registerAllProvider(const std::vector<std::shared_ptr<Config>> &configs);
        void initProviders(const std::vector<std::shared_ptr<Config>> &configs);

        // 针对不同类型的配置进行特定初始化
        bool initAPIModelProviders(const std::string &modelName, const std::shared_ptr<APIConfig> &apiConfig);
        bool initOllamaModelProviders(const std::string &modelName, const std::shared_ptr<OllamaConfig> &config);

    private:
        bool _initialized = false;

        // Key: 模型名称, Value: 配置信息 (用于 sendMessage 时提取 temperature 等参数)
        std::unordered_map<std::string, std::shared_ptr<Config>> _modelConfigs;

        // 核心组件
        LLMManager _llmManager;

    public:
        // 将 SessionManager 设为 public 或提供访问器，视具体设计而定
        // 这里为了方便测试和直接访问，暂时设为 public
        SessionManager _sessionManager;
    };
} // end ai_chat_sdk