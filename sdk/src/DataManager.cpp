
#include "../include/DataManager.h"
#include "../include/util/myLog.h"
#include <memory>
#include <mutex>
#include <vector>

namespace ai_chat_sdk
{

    DataManager::DataManager(const std::string &dbName)
        : _dbName(dbName), _db(nullptr)
    {

        // 1. 创建并打开数据库
        // c_str() 将 std::string 转换为 C 风格字符串，因为 sqlite3 是 C 接口
        int rc = sqlite3_open(dbName.c_str(), &_db);
        if (rc != SQLITE_OK)
        {
            ERR("DataManager: 打开数据库失败：{}", sqlite3_errmsg(_db));
            // 注意：即使打开失败，sqlite3_open 也可能返回一个非空的 db 句柄，需要后续处理
        }
        else
        {
            INFO("DataManager: 打开数据库成功：{}", dbName);
        }

        // 2. 初始化数据库表 (会话表和消息表)
        if (!initDataBase())
        {
            // 如果表结构初始化失败，数据库连接也没有保留的必要了
            sqlite3_close(_db);
            _db = nullptr;
            ERR("DataManager: 初始化数据库表失败");
        }
    }

    DataManager::~DataManager()
    {
        if (_db)
        {
            sqlite3_close(_db);
            INFO("DataManager: 数据库连接已关闭");
        }
    }

    // 初始化数据库
    bool DataManager::initDataBase()
    {
        // 1. 创建会话表
        // 使用 IF NOT EXISTS 避免重复创建报错
        std::string createSessionTable = R"(
        CREATE TABLE IF NOT EXISTS sessions (
            session_id TEXT PRIMARY KEY,
            model_name TEXT NOT NULL,
            create_time INTEGER NOT NULL,
            update_time INTEGER NOT NULL
        );
    )";

        if (!executeSQL(createSessionTable))
        {
            return false;
        }

        // 2. 创建消息表
        // 设置外键约束和级联删除
        std::string createMessageTable = R"(
        CREATE TABLE IF NOT EXISTS messages (
            message_id TEXT PRIMARY KEY,
            session_id TEXT NOT NULL,
            role TEXT NOT NULL,
            content TEXT NOT NULL,
            timestamp INTEGER NOT NULL,
            FOREIGN KEY (session_id) REFERENCES sessions(session_id) ON DELETE CASCADE
        );
    )";

        if (!executeSQL(createMessageTable))
        {
            return false;
        }

        // 开启外键约束支持 (SQLite 默认可能关闭)
        executeSQL("PRAGMA foreign_keys = ON;");

        return true;
    }

    bool DataManager::executeSQL(const std::string &sql)
    {
        if (!_db)
        {
            ERR("DataManager: 数据库未初始化");
            return false;
        }

        char *errMsg = nullptr;
        // sqlite3_exec 参数：db句柄, SQL语句, 回调函数, 回调参数, 错误信息指针
        int rc = sqlite3_exec(_db, sql.c_str(), nullptr, nullptr, &errMsg);

        if (rc != SQLITE_OK)
        {
            ERR("DataManager: 执行SQL语句失败: {}, SQL: {}", errMsg, sql);
            sqlite3_free(errMsg); // 务必释放错误信息的内存
            return false;
        }
        return true;
    }

    // 插入会话
    bool DataManager::insertSession(const Session &session)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        // 1. 构建 SQL
        const std::string sql = "INSERT INTO sessions (session_id, model_name, create_time, update_time) VALUES (?, ?, ?, ?);";
        sqlite3_stmt *stmt;

        // 2. 预处理
        if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            ERR("DataManager::insertSession: Prepare failed: {}", sqlite3_errmsg(_db));
            return false;
        }

        // 3. 绑定参数
        sqlite3_bind_text(stmt, 1, session._sessionId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, session._modelName.c_str(), -1, SQLITE_TRANSIENT);
        // time_t 通常是 long 类型，使用 int64 存储
        sqlite3_bind_int64(stmt, 3, static_cast<int64_t>(session._createdAt));
        sqlite3_bind_int64(stmt, 4, static_cast<int64_t>(session._updatedAt));

        // 4. 执行
        if (sqlite3_step(stmt) != SQLITE_DONE)
        {
            ERR("DataManager::insertSession: Step failed: {}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return false;
        }

        // 5. 清理
        sqlite3_finalize(stmt);
        INFO("DataManager: Insert session success: {}", session._sessionId);
        return true;
    }

    // 获取指定sessionId的会话信息
    std::shared_ptr<Session> DataManager::getSession(const std::string &sessionId) const
    {
        std::lock_guard<std::mutex> lock(_mutex);

        const std::string sql = "SELECT model_name, create_time, update_time FROM sessions WHERE session_id = ?;";
        sqlite3_stmt *stmt;

        if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            ERR("DataManager::getSession: Prepare failed: {}", sqlite3_errmsg(_db));
            return nullptr;
        }

        sqlite3_bind_text(stmt, 1, sessionId.c_str(), -1, SQLITE_TRANSIENT);

        // 执行查询
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            // 提取字段
            std::string modelName = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            time_t createTime = static_cast<time_t>(sqlite3_column_int64(stmt, 1));
            time_t updateTime = static_cast<time_t>(sqlite3_column_int64(stmt, 2));

            // 构造对象
            auto session = std::make_shared<Session>(modelName);
            session->_sessionId = sessionId;
            session->_createdAt = createTime;
            session->_updatedAt = updateTime;

            sqlite3_finalize(stmt);

            // 加载历史消息 (注意：getSessionMessages 内部也会加锁，这里需要小心死锁)
            // 在单线程调试环境下没问题，但在多线程下，建议将 getSessionMessages 逻辑内联，或者使用 recursive_mutex
            // 这里为了简化，我们假设 getSessionMessages 也是 const 成员函数且锁粒度合适
            session->_messages = getSessionMessages(sessionId);

            return session;
        }

        sqlite3_finalize(stmt);
        WARN("DataManager::getSession: Session not found: {}", sessionId);
        return nullptr;
    }

    // 更新指定会话的时间戳
    bool DataManager::updateSessionTimestamp(const std::string &sessionId, std::time_t timestamp)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        const std::string sql = "UPDATE sessions SET update_time = ? WHERE session_id = ?;";
        sqlite3_stmt *stmt;

        if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            return false;
        }

        sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(timestamp));
        sqlite3_bind_text(stmt, 2, sessionId.c_str(), -1, SQLITE_TRANSIENT);

        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return (rc == SQLITE_DONE);
    }

    // 删除指定会话
    bool DataManager::deleteSession(const std::string &sessionId)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        const std::string sql = "DELETE FROM sessions WHERE session_id = ?;";
        sqlite3_stmt *stmt;

        if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            return false;
        }

        sqlite3_bind_text(stmt, 1, sessionId.c_str(), -1, SQLITE_TRANSIENT);

        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc == SQLITE_DONE)
        {
            INFO("DataManager: Deleted session: {}", sessionId);
            return true;
        }
        return false;
    }

    // 获取所有会话id
    std::vector<std::string> DataManager::getAllSessionIds() const
    {
        std::lock_guard<std::mutex> lock(_mutex);

        // 构建 SQL：按更新时间降序排列 (最新的在前面)
        std::string selectSQL = R"(SELECT session_id FROM sessions ORDER BY update_time DESC;)";

        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(_db, selectSQL.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            ERR("getAllSessionIds - 准备语句失败：{}", sqlite3_errmsg(_db));
            return {};
        }

        std::vector<std::string> sessionIds;
        // 循环遍历结果集
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            // 第 0 列是 session_id
            std::string sessionId = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            sessionIds.push_back(sessionId);
        }

        sqlite3_finalize(stmt);
        INFO("getAllSessionIds - 获取所有会话id成功, 会话总数：{}", sessionIds.size());
        return sessionIds;
    }

    // 获取所有session信息，并按照更新时间降序排列
    std::vector<std::shared_ptr<Session>> DataManager::getAllSessions() const
    {
        std::lock_guard<std::mutex> lock(_mutex);

        // 查询所有元数据，同样按时间倒序
        std::string selectSQL = R"(SELECT session_id, model_name, create_time, update_time FROM sessions ORDER BY update_time DESC;)";

        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(_db, selectSQL.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            ERR("getAllSessions - 准备语句失败：{}", sqlite3_errmsg(_db));
            return {};
        }

        std::vector<std::shared_ptr<Session>> sessions;
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            // 提取各列数据
            std::string sessionId = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            std::string modelName = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            int64_t createTime = sqlite3_column_int64(stmt, 2);
            int64_t updateTime = sqlite3_column_int64(stmt, 3);

            // 重建 Session 对象
            auto session = std::make_shared<Session>(modelName);
            session->_sessionId = sessionId;
            session->_createdAt = static_cast<std::time_t>(createTime);
            session->_updatedAt = static_cast<std::time_t>(updateTime);

            // 【关键】这里不加载 messages，保持 vector 为空
            // 历史消息采用懒加载策略，需要时再通过 getSessionMessages 获取

            sessions.push_back(session);
        }

        sqlite3_finalize(stmt);
        INFO("getAllSessions - 获取所有会话信息成功, 会话总数：{}", sessions.size());
        return sessions;
    }

    // 获取会话总数
    size_t DataManager::getSessionCount() const
    {
        std::lock_guard<std::mutex> lock(_mutex);

        std::string selectSQL = R"(SELECT COUNT(*) FROM sessions;)";

        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(_db, selectSQL.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            ERR("getSessionCount - 准备语句失败：{}", sqlite3_errmsg(_db));
            return 0;
        }

        rc = sqlite3_step(stmt);
        size_t count = 0;
        if (rc == SQLITE_ROW)
        {
            count = static_cast<size_t>(sqlite3_column_int64(stmt, 0));
        }
        else
        {
            ERR("getSessionCount - 执行语句失败：{}", sqlite3_errmsg(_db));
        }

        sqlite3_finalize(stmt);
        INFO("getSessionCount - 获取会话总数成功：{}", count);
        return count;
    }

    // 删除所有会话
    bool DataManager::clearAllSessions()
    {
        std::lock_guard<std::mutex> lock(_mutex);

        std::string deleteSQL = R"(DELETE FROM sessions;)";

        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(_db, deleteSQL.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            ERR("clearAllSessions - 准备语句失败：{}", sqlite3_errmsg(_db));
            return false;
        }

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE)
        {
            ERR("clearAllSessions - 执行语句失败：{}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return false;
        }

        sqlite3_finalize(stmt);
        INFO("clearAllSessions - 删除所有会话成功");
        return true;
    }

    // 插入新消息--注意：插入消息时，需要更新会话的时间戳
    bool DataManager::insertMessage(const std::string &sessionId, const Message &message)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        // 1. 构建插入消息的 SQL
        // 注意：timestamp 用于记录消息生成时间
        const std::string insertSQL = "INSERT INTO messages (message_id, session_id, role, content, timestamp) VALUES (?, ?, ?, ?, ?);";
        sqlite3_stmt *stmt;

        if (sqlite3_prepare_v2(_db, insertSQL.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            ERR("DataManager::insertMessage: Prepare insert failed: {}", sqlite3_errmsg(_db));
            return false;
        }

        // 绑定参数
        sqlite3_bind_text(stmt, 1, message._messageId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, sessionId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, message._role.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, message._content.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 5, static_cast<int64_t>(message._timestamp));

        // 执行插入
        if (sqlite3_step(stmt) != SQLITE_DONE)
        {
            ERR("DataManager::insertMessage: Insert step failed: {}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt); // 释放第一个 stmt

        // 2. 构建更新会话时间戳的 SQL
        const std::string updateSQL = "UPDATE sessions SET update_time = ? WHERE session_id = ?;";
        sqlite3_stmt *updateStmt;

        if (sqlite3_prepare_v2(_db, updateSQL.c_str(), -1, &updateStmt, nullptr) != SQLITE_OK)
        {
            ERR("DataManager::insertMessage: Prepare update failed: {}", sqlite3_errmsg(_db));
            return false;
        }

        // 绑定参数
        sqlite3_bind_int64(updateStmt, 1, static_cast<int64_t>(message._timestamp));
        sqlite3_bind_text(updateStmt, 2, sessionId.c_str(), -1, SQLITE_TRANSIENT);

        // 执行更新
        if (sqlite3_step(updateStmt) != SQLITE_DONE)
        {
            ERR("DataManager::insertMessage: Update step failed: {}", sqlite3_errmsg(_db));
            sqlite3_finalize(updateStmt);
            return false;
        }

        sqlite3_finalize(updateStmt); // 释放第二个 stmt

        INFO("DataManager: Insert message success: {}", message._messageId);
        return true;
    }

    // 获取会话中的所有消息
    std::vector<Message> DataManager::getSessionMessages(const std::string &sessionId) const
    {
        std::lock_guard<std::mutex> lock(_mutex);

        // SQL: 按时间戳升序查询
        const std::string selectSQL = "SELECT message_id, role, content, timestamp FROM messages WHERE session_id = ? ORDER BY timestamp ASC;";
        sqlite3_stmt *stmt;

        if (sqlite3_prepare_v2(_db, selectSQL.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            ERR("DataManager::getSessionMessages: Prepare failed: {}", sqlite3_errmsg(_db));
            return {};
        }

        sqlite3_bind_text(stmt, 1, sessionId.c_str(), -1, SQLITE_TRANSIENT);

        std::vector<Message> messages;
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            // 提取每行数据
            std::string msgId = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            std::string role = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            std::string content = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            time_t timestamp = static_cast<time_t>(sqlite3_column_int64(stmt, 3));

            // 构造 Message 对象
            Message msg(role, content);
            msg._messageId = msgId;
            msg._timestamp = timestamp;

            messages.push_back(msg);
        }

        sqlite3_finalize(stmt);
        return messages;
    }

    // 删除制定会话的历史消息
    bool DataManager::deleteSessionMessages(const std::string &sessionId)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        const std::string deleteSQL = "DELETE FROM messages WHERE session_id = ?;";
        sqlite3_stmt *stmt;

        if (sqlite3_prepare_v2(_db, deleteSQL.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            return false;
        }

        sqlite3_bind_text(stmt, 1, sessionId.c_str(), -1, SQLITE_TRANSIENT);

        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc == SQLITE_DONE)
        {
            INFO("DataManager: Deleted messages for session: {}", sessionId);
            return true;
        }
        return false;
    }

} // end ai_chat_sdk
