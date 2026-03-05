#pragma once
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <memory>
#include "DataManager.h"
#include "common.h"

namespace ai_chat_sdk
{

    class SessionManager
    {
    public:
        // 构造函数：可指定数据库名称
        SessionManager(const std::string &dbName = "chatDB.db");
        ~SessionManager();

        // 1. 创建会话
        // param modelName: 该会话关联的模型 (如 deepseek-chat)
        // return: 生成的唯一 Session ID
        std::string createSession(const std::string &modelName);

        // 2. 获取指定会话
        // return: 会话对象的智能指针，若不存在返回 nullptr
        std::shared_ptr<Session> getSession(const std::string &sessionId);

        // 3. 添加消息
        // 将用户提问或 AI 回复追加到会话历史中
        bool addMessage(const std::string &sessionId, const Message &message);

        // 4. 获取历史消息
        // 用于构造 API 请求的 Context
        std::vector<Message> getHistoryMessages(const std::string &sessionId) const;

        // 5. 获取所有会话列表
        // 返回所有会话的 ID 列表 (用于前端展示会话侧边栏)
        std::vector<std::string> getSessionList() const;

        // 6. 删除会话
        bool deleteSession(const std::string &sessionId);

        // 7. 清空所有会话
        void clearAllSessions();

        // 8. 获取会话总数
        size_t getSessionCount() const;

        // 9. 更新会话时间戳 (每次交互后调用)
        void updateSessionTimestamp(const std::string &sessionId);

    private:
        // 内部辅助方法：生成唯一 ID
        std::string generateSessionId();
        std::string generateMessageId(size_t messageCounter);

    private:
        // 核心存储容器
        // Key: sessionId, Value: Session 对象指针
        std::unordered_map<std::string, std::shared_ptr<Session>> _sessions;

        // 线程安全锁
        // mutable 关键字允许在 const 成员函数中修改此变量 (加锁/解锁)
        mutable std::mutex _mutex;

        // 会话计数器 (原子操作，用于生成 ID)
        std::atomic<int64_t> _sessionCounter = {0};

        // 数据持久化管理 (后续章节实现)
        DataManager _dataManager;
    };

} // namespace ai_chat_sdk