#include "../include/SessionManager.h"
#include "../include/util/myLog.h"
#include <iomanip>
#include <sstream>
#include <vector>

namespace ai_chat_sdk
{

    SessionManager::SessionManager(const std::string &dbName) : _dataManager(dbName)
    {
        // 从数据库加载会话到内存

        auto sessions = _dataManager.getAllSessions();
        for (auto &session : sessions)
        {
            _sessions[session->_sessionId] = session;
        }
        // 更新原子计数器，避免 ID 冲突
        // _sessionCounter = sessions.size();

        INFO("SessionManager initialized with DB: {}", dbName);
    }

    // 生成会话id  会话id格式：session_时间戳_会话计数
    std::string SessionManager::generateSessionId()
    {
        // 1. 计数器原子自增 (保证多线程安全)
        _sessionCounter.fetch_add(1);

        // 2. 获取当前时间戳
        std::time_t time = std::time(nullptr);

        // 3. 格式化拼接
        std::ostringstream os;
        os << "session_" << time << "_"
           << std::setw(8) << std::setfill('0') << _sessionCounter;

        return os.str();
    }

    // 生成消息id  msg_1234567890_00000001
    std::string SessionManager::generateMessageId(size_t messageCounter)
    {
        messageCounter++; // 本次新消息是第 N+1 条
        std::time_t time = std::time(nullptr);

        std::ostringstream os;
        os << "msg_" << time << "_"
           << std::setw(8) << std::setfill('0') << messageCounter;

        return os.str();
    }

    // 创建会话，提供模型名称
    std::string SessionManager::createSession(const std::string &modelName)
    {
        std::lock_guard<std::mutex> lock(_mutex); // 自动加锁解锁

        std::string sessionId = generateSessionId();

        auto session = std::make_shared<Session>(modelName);
        session->_sessionId = sessionId;
        session->_createdAt = std::time(nullptr);
        session->_updatedAt = session->_createdAt;

        _sessions[sessionId] = session;

        INFO("Created new session: {}", sessionId);

        _dataManager.insertSession(*session); // 数据库持久化
        return sessionId;
    }

    // 通过会话ID获取会话信息
    std::shared_ptr<Session> SessionManager::getSession(const std::string &sessionId)
    {
        // 先在内存中查找
        _mutex.lock();
        auto it = _sessions.find(sessionId);
        if (it != _sessions.end())
        {
            _mutex.unlock();
            // 获取当前会话的历史消息
            it->second->_messages = _dataManager.getSessionMessages(sessionId);
            return it->second;
        }
        _mutex.unlock();

        // 内存中没有找到，从数据库中查找
        auto session = _dataManager.getSession(sessionId);
        if (session)
        {
            _mutex.lock();
            auto it = _sessions.find(sessionId);
            if (it == _sessions.end())
            {
                // 内存中没有找到，将会话添加到会话列表
                _sessions[sessionId] = session;
            }
            _mutex.unlock();

            // 获取当前会话的历史消息
            session->_messages = _dataManager.getSessionMessages(sessionId);
            return session;
        }

        WARN("sessionId = {} not found", sessionId);
        return nullptr;
    }

    // 往某个会话中添加消息
    bool SessionManager::addMessage(const std::string &sessionId, const Message &message)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        auto it = _sessions.find(sessionId);
        if (it == _sessions.end())
        {
            WARN("addMessage failed: Session {} not found", sessionId);
            return false;
        }

        // 构造完整消息对象
        Message msg = message;
        msg._messageId = generateMessageId(it->second->_messages.size());
        msg._timestamp = std::time(nullptr);

        // 更新会话状态
        it->second->_messages.push_back(msg);
        it->second->_updatedAt = std::time(nullptr);

        INFO("Added message to session {}: {}", sessionId, msg._content);

        _dataManager.insertMessage(sessionId, msg); // 数据库持久化
        return true;
    }

    // 获取某个会话的所有历史消息
    std::vector<Message> SessionManager::getHistoryMessages(const std::string &sessionId) const
    {
        std::lock_guard<std::mutex> lock(_mutex);

        // 1. 尝试从内存获取
        auto it = _sessions.find(sessionId);
        if (it != _sessions.end())
        {
            return it->second->_messages;
        }

        // 2. 内存没有，尝试从数据库加载
        return _dataManager.getSessionMessages(sessionId);

        WARN("getHistoryMessages: Session {} not found", sessionId);
        return {};
    }

    // 更新会话时间戳
    void SessionManager::updateSessionTimestamp(const std::string &sessionId)
    {
        _mutex.lock();
        auto it = _sessions.find(sessionId);
        if (it != _sessions.end())
        {
            it->second->_updatedAt = std::time(nullptr);
        }
        _mutex.unlock();

        // 更新数据库中的会话时间戳
        _dataManager.updateSessionTimestamp(sessionId, it->second->_updatedAt);
    }

    // 获取所有会话列表 (按更新时间降序排列)
    std::vector<std::string> SessionManager::getSessionList() const
    {
        // 1. 获取数据库中的会话
        auto sessions = _dataManager.getAllSessions();

        std::lock_guard<std::mutex> lock(_mutex);

        // 2. 构建临时容器，用于排序
        // pair<时间戳, 会话指针>
        std::vector<std::pair<std::time_t, std::shared_ptr<Session>>> temp;
        temp.reserve(_sessions.size());

        // 3. 将内存中的会话加入临时容器
        for (const auto &pair : _sessions)
        {
            temp.emplace_back(pair.second->_updatedAt, pair.second);
        }

        // 4. 将数据库中的会话加入临时容器 (去重逻辑)
        for (const auto &session : sessions)
        {
            if (_sessions.find(session->_sessionId) == _sessions.end())
            {
                temp.emplace_back(session->_updatedAt, session);
            }
        }

        // 5. 执行排序：按时间戳降序 (最新的在前面)
        std::sort(temp.begin(), temp.end(),
                  [](const auto &a, const auto &b)
                  {
                      return a.first > b.first;
                  });

        // 6. 提取排序后的 Session ID
        std::vector<std::string> sessionIds;
        sessionIds.reserve(temp.size());
        for (const auto &pair : temp)
        {
            sessionIds.push_back(pair.second->_sessionId);
        }

        return sessionIds;
    }

    // 删除某个会话
    bool SessionManager::deleteSession(const std::string &sessionId)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        auto it = _sessions.find(sessionId);
        if (it == _sessions.end())
        {
            return false; // 会话不存在
        }

        // 1. 从内存中删除
        _sessions.erase(it);

        // 2. 从数据库中删除 (持久化)
        _dataManager.deleteSession(sessionId);

        INFO("Deleted session: {}", sessionId);
        return true;
    }

    // 清空所有会话
    void SessionManager::clearAllSessions()
    {
        std::lock_guard<std::mutex> lock(_mutex);

        // 1. 清空内存
        _sessions.clear();

        // 2. 清空数据库
        _dataManager.clearAllSessions();

        INFO("All sessions cleared.");
    }

    // 获取会话总数
    size_t SessionManager::getSessionCount() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _sessions.size();
    }

} // end ai_chat_sdk
