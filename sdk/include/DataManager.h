#pragma once
#include <memory>
#include <sqlite3.h>
#include <string>
#include <mutex>
#include "common.h"

namespace ai_chat_sdk
{

    class DataManager
    {
    public:
        // 构造函数：传入数据库文件名（如 chat.db）
        DataManager(const std::string &dbName);

        // 析构函数：负责关闭数据库连接
        ~DataManager();

        // ================= Session 相关操作 =================

        /**
         * @brief 插入新会话
         * @param session 会话对象
         * @return true 成功, false 失败
         */
        bool insertSession(const Session &session);

        /**
         * @brief 获取指定会话信息
         * @param sessionId 会话ID
         * @return 会话对象的智能指针，若不存在返回 nullptr
         */
        std::shared_ptr<Session> getSession(const std::string &sessionId) const;

        /**
         * @brief 更新指定会话的时间戳
         * 当有新消息产生时调用此方法，使会话在列表中排到前面
         */
        bool updateSessionTimestamp(const std::string &sessionId, std::time_t timestamp);

        /**
         * @brief 删除指定会话
         * 注意：这将触发级联删除，同时删除该会话关联的所有消息
         */
        bool deleteSession(const std::string &sessionId);

        /**
         * @brief 获取所有会话ID
         * 通常用于程序启动时快速加载会话列表索引
         */
        std::vector<std::string> getAllSessionIds() const;

        /**
         * @brief 获取所有会话完整信息
         */
        std::vector<std::shared_ptr<Session>> getAllSessions() const;

        /**
         * @brief 清空所有会话
         * 慎用操作，将清空整个数据库的业务数据
         */
        bool clearAllSessions();

        /**
         * @brief 获取会话总数
         */
        size_t getSessionCount() const;

        // ================= Message 相关操作 =================

        /**
         * @brief 插入新消息
         * 注意：插入消息时，业务逻辑层通常需要同步调用 updateSessionTimestamp
         */
        bool insertMessage(const std::string &sessionId, const Message &message);

        /**
         * @brief 获取指定会话的历史消息
         * 用于加载聊天记录上下文，发送给大模型
         */
        std::vector<Message> getSessionMessages(const std::string &sessionId) const;

        /**
         * @brief 删除指定会话的所有消息
         */
        bool deleteSessionMessages(const std::string &sessionId);

    private:
        /**
         * @brief 初始化数据库
         * 负责创建 session_tb 和 message_tb 表
         */
        bool initDataBase();

        /**
         * @brief 执行无返回值的 SQL 语句工具函数
         * 封装 sqlite3_exec，用于简化 CREATE/INSERT/DELETE 操作
         */
        bool executeSQL(const std::string &sql);

    private:
        sqlite3 *_db = nullptr; // 数据库连接句柄
        std::string _dbName;    // 数据库文件名

        // 线程安全锁
        // 使用 mutable 关键字，允许在 const 成员函数（如 getSession）中被加锁修改
        mutable std::mutex _mutex;
    };

} // namespace ai_chat_sdk
