#include "ChatServer.h"
#include <ai_chat_sdk/util/myLog.h>
#include <jsoncpp/json/json.h>

namespace ai_chat_server
{

    ChatServer::ChatServer(const ServerConfig &config) : _config(config)
    {
        // 1. 实例化 ChatSDK
        _chatSDK = std::make_shared<ai_chat_sdk::ChatSDK>();

        // 2. 构造各类模型的配置信息
        // ---------------- DeepSeek ----------------
        auto deepseekConfig = std::make_shared<ai_chat_sdk::APIConfig>();
        deepseekConfig->_modelName = "deepseek-chat";
        deepseekConfig->_apiKey = config.deepseekAPIKey;
        deepseekConfig->_temperature = config.temperature;
        deepseekConfig->_maxTokens = config.maxTokens;

        // ---------------- ChatGPT ----------------
        auto chatGPTConfig = std::make_shared<ai_chat_sdk::APIConfig>();
        chatGPTConfig->_modelName = "gpt-4o-mini";
        chatGPTConfig->_apiKey = config.chatGPTAPIKey;
        chatGPTConfig->_temperature = config.temperature;
        chatGPTConfig->_maxTokens = config.maxTokens;

        // ---------------- Gemini ----------------
        auto geminiConfig = std::make_shared<ai_chat_sdk::APIConfig>();
        geminiConfig->_modelName = "gemini-2.5-flash";
        geminiConfig->_apiKey = config.geminiAPIKey;
        geminiConfig->_temperature = config.temperature;
        geminiConfig->_maxTokens = config.maxTokens;

        // ---------------- Ollama (本地模型) ----------------
        auto ollamaConfig = std::make_shared<ai_chat_sdk::OllamaConfig>();
        ollamaConfig->_modelName = config.ollamaModelName;
        ollamaConfig->_modelDesc = config.ollamaModelDesc;
        ollamaConfig->_endpoint = config.ollamaEndpoint;
        ollamaConfig->_temperature = config.temperature;
        ollamaConfig->_maxTokens = config.maxTokens;

        // 3. 聚合配置并初始化 SDK
        std::vector<std::shared_ptr<ai_chat_sdk::Config>> modelConfigs = {
            deepseekConfig, chatGPTConfig, geminiConfig, ollamaConfig};

        INFO("ChatServer: Start initializing ChatSDK models...");
        if (!_chatSDK->initModels(modelConfigs))
        {
            ERR("ChatServer: ChatSDK init Failed!!!");
            return;
        }
        INFO("ChatServer: ChatSDK models init success!!!");

        // 4. 初始化 HTTP 服务器引擎
        _chatServer = std::make_unique<httplib::Server>();
        if (!_chatServer)
        {
            ERR("ChatServer: HTTP Server instantiation failed!!!");
            return;
        }
    }

    bool ChatServer::start()
    {
        // 1. 状态防重检测：防止重复启动
        if (_isRunning.load())
        {
            ERR("ChatServer is already running!!!");
            return false;
        }

        // 2. 设置 HTTP 路由规则
        // 将 /api/... 等接口请求映射到对应的处理函数上 (下一章详细实现)
        setHttpRoutes();

        // 3. 设置静态资源的挂载路径 (对接前端页面)
        // 参数1: URL 挂载点，参数2: 本地相对目录
        // 当用户访问 http://ip:port/ 时，httplib 会自动返回 ./www/index.html
        _chatServer->set_mount_point("/", "./www");

        // 4. 在独立线程中启动监听，避免阻塞主线程
        std::thread serverThread([this]()
                                 {
        // listen 是一个阻塞调用，直到服务器停止才会返回
        _chatServer->listen(_config.host, _config.port);
        INFO("ChatServer listen stopped on {} :{}", _config.host, _config.port); });

        // 线程分离，让其在后台独立运行
        serverThread.detach();

        // 5. 更新运行状态标志
        _isRunning.store(true);
        INFO("ChatServer start success!!! Listening on {}:{}", _config.host, _config.port);

        return true;
    }

    void ChatServer::stop()
    {
        // 1. 状态检测
        if (!_isRunning.load())
        {
            ERR("ChatServer is not running!!!");
            return;
        }

        // 2. 停止 HTTP 引擎
        if (_chatServer)
        {
            _chatServer->stop(); // 这个调用会让之前阻塞的 listen() 返回
        }

        // 3. 更新状态
        _isRunning.store(false);
        INFO("ChatServer stop success!!!");
    }

    bool ChatServer::isRunning() const
    {
        return _isRunning.load();
    }

    // 构造通用响应体
    std::string ChatServer::buildResponse(const std::string &message, bool success)
    {
        Json::Value responseJson;
        responseJson["success"] = success;
        responseJson["message"] = message;

        // 序列化为字符串
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = ""; // 紧凑格式，节省带宽
        return Json::writeString(writerBuilder, responseJson);
    }

    // 处理创建会话请求
    void ChatServer::handleCreateSessionRequest(const httplib::Request &request, httplib::Response &response)
    {
        // 1. 获取并解析请求参数
        Json::Value requestJson;
        Json::Reader reader; // 注意：部分新版 jsoncpp 推荐使用 CharReaderBuilder，这里沿用板书写法

        // 反序列化校验
        if (!reader.parse(request.body, requestJson))
        {
            std::string errorJsonStr = buildResponse("parse request body failed, json format error", false);
            // 400 Bad Request: 客户端发送的请求有语法错误，服务器无法理解
            response.status = 400;
            response.set_content(errorJsonStr, "application/json");
            return;
        }

        // 2. 提取模型名称，若客户端未传，则默认使用 deepseek-chat
        std::string modelName = requestJson.get("model", "deepseek-chat").asString();

        // 3. 调用底层 ChatSDK 创建会话
        std::string sessionID = _chatSDK->createSession(modelName);

        if (sessionID.empty())
        {
            std::string errorJsonStr = buildResponse("create session failed (Internal Error)", false);
            // 500 Internal Server Error: 服务器内部发生错误，无法完成请求
            response.status = 500;
            response.set_content(errorJsonStr, "application/json");
            return;
        }

        // 4. 构建成功的响应体
        Json::Value dataJson;
        dataJson["session_id"] = sessionID;
        dataJson["model"] = modelName;

        Json::Value responseJson;
        responseJson["success"] = true;
        responseJson["message"] = "create session success";
        responseJson["data"] = dataJson;

        // 序列化
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "";
        std::string responseJsonStr = Json::writeString(writerBuilder, responseJson);

        // 5. 设置 HTTP 响应
        response.status = 200; // 200 OK: 请求已成功
        response.set_content(responseJsonStr, "application/json");
    }

    // 处理获取会话列表请求
    void ChatServer::handleGetSessionListsRequest(const httplib::Request &request, httplib::Response &response)
    {
        // 1. 从 SDK 获取所有会话的 ID 列表 (已按更新时间降序排列)
        std::vector<std::string> sessionIDs = _chatSDK->getSessionList();

        // 2. 构建核心数据数组 (data 字段)
        Json::Value dataArray(Json::arrayValue);

        for (const auto &sessionID : sessionIDs)
        {
            auto session = _chatSDK->getSession(sessionID);
            if (session)
            {
                Json::Value sessionJson;
                sessionJson["id"] = session->_sessionId;
                sessionJson["model"] = session->_modelName;

                // 时间戳通常使用 int64_t 传递，避免精度丢失或前端解析错误
                sessionJson["created_at"] = static_cast<int64_t>(session->_createdAt);
                sessionJson["updated_at"] = static_cast<int64_t>(session->_updatedAt);

                // 统计对话次数
                sessionJson["message_count"] = static_cast<int>(session->_messages.size());

                // 提取第一条正文消息作为会话的 UI 标题
                if (!session->_messages.empty())
                {
                    sessionJson["first_user_message"] = session->_messages.front()._content;
                }
                else
                {
                    sessionJson["first_user_message"] = "新会话"; // 如果没有消息的默认提示
                }

                dataArray.append(sessionJson);
            }
        }

        // 3. 构建完整的标准响应体
        Json::Value responseJson;
        responseJson["success"] = true;
        responseJson["message"] = "get session lists success";
        responseJson["data"] = dataArray;

        // 4. 序列化并填充 HTTP 响应
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = ""; // 使用紧凑的 JSON 格式节省带宽
        std::string responseJsonStr = Json::writeString(writerBuilder, responseJson);

        response.status = 200; // 成功
        response.set_content(responseJsonStr, "application/json");
    }

    // 处理获取可用模型列表请求
    void ChatServer::handleGetModelListsRequest(const httplib::Request &request, httplib::Response &response)
    {
        // 1. 获取底层 SDK 支持且已初始化成功的模型列表
        auto modelLists = _chatSDK->getAvailableModels();

        // 2. 构建 data 数组
        Json::Value dataArray(Json::arrayValue);
        for (const auto &modelInfo : modelLists)
        {
            Json::Value modelJson;
            // 填入模型名称和描述
            modelJson["name"] = modelInfo._modelName;
            modelJson["desc"] = modelInfo._modelDesc;
            dataArray.append(modelJson);
        }

        // 3. 构建完整的标准响应体
        Json::Value responseJson;
        responseJson["success"] = true;
        responseJson["message"] = "get model lists success";
        responseJson["data"] = dataArray;

        // 4. 序列化并填充 HTTP 响应
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "";
        std::string responseJsonStr = Json::writeString(writerBuilder, responseJson);

        response.status = 200; // HTTP 状态码 200 OK
        response.set_content(responseJsonStr, "application/json");
    }

    // 处理删除会话请求
    void ChatServer::handleDeleteSessionRequest(const httplib::Request &request, httplib::Response &response)
    {
        // 1. 获取会话 ID。注意：这里通过正则匹配提取路径参数
        std::string sessionId = request.matches[1];

        // 2. 调用 SDK 删除会话
        bool ret = _chatSDK->deleteSession(sessionId);

        // 3. 构造响应与状态码分发
        if (ret)
        {
            // 删除成功，不需要返回额外 data，只需 success 和 message
            std::string successJsonStr = buildResponse("delete session success", true);
            response.status = 200;
            response.set_content(successJsonStr, "application/json");
        }
        else
        {
            // 删除失败，说明客户端请求的资源不存在
            std::string errorJsonStr = buildResponse("delete session failed, session not found", false);
            response.status = 404; // 404 Not Found
            response.set_content(errorJsonStr, "application/json");
        }
    }

    // 处理获取历史消息请求
    void ChatServer::handleGetHistoryMessagesRequest(const httplib::Request &request, httplib::Response &response)
    {
        // 1. 从路径中获取会话 ID
        std::string sessionId = request.matches[1];

        // 2. 获取会话对象
        // SDK 内部会自动从 SQLite 数据库懒加载该会话的历史消息
        auto session = _chatSDK->getSession(sessionId);
        if (!session)
        {
            std::string errorJsonStr = buildResponse("session not found", false);
            response.status = 404; // 404 Not Found
            response.set_content(errorJsonStr, "application/json");
            return;
        }

        // 3. 构建历史消息列表 (JSON 数组)
        Json::Value dataArray(Json::arrayValue);
        for (const auto &message : session->_messages)
        {
            Json::Value messageJson;
            messageJson["id"] = message._messageId;
            messageJson["role"] = message._role;       // "user" 或 "assistant"
            messageJson["content"] = message._content; // 具体的文本内容

            // 时间戳强制转换为 int64_t 以确保 JSON 序列化无误
            messageJson["timestamp"] = static_cast<int64_t>(message._timestamp);

            dataArray.append(messageJson);
        }

        // 4. 构建标准响应体
        Json::Value responseJson;
        responseJson["success"] = true;
        responseJson["message"] = "get history messages success";
        responseJson["data"] = dataArray;

        // 5. 序列化为紧凑字符串
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "";
        std::string responseJsonStr = Json::writeString(writerBuilder, responseJson);

        // 6. 填充 HTTP 响应
        response.status = 200; // 200 OK
        response.set_content(responseJsonStr, "application/json");
    }

    // 处理发送消息请求 - 全量返回
    void ChatServer::handleSendMessageRequest(const httplib::Request &request, httplib::Response &response)
    {
        // 1. 获取并解析请求参数
        // 客户端发来的数据在 request.body 中，我们需要将其反序列化为 JSON 对象
        Json::Value requestJson;
        Json::Reader reader; // 注：也可使用现代的 Json::CharReaderBuilder

        if (!reader.parse(request.body, requestJson))
        {
            // 如果 JSON 格式不合法，返回 400 Bad Request
            std::string errorJsonStr = buildResponse("parse request body failed, json format error", false);
            response.status = 400;
            response.set_content(errorJsonStr, "application/json");
            return;
        }

        // 2. 提取并校验核心参数
        std::string sessionId = requestJson["session_id"].asString();
        std::string message = requestJson["message"].asString();

        if (sessionId.empty() || message.empty())
        {
            std::string errorJsonStr = buildResponse("session_id or message is empty", false);
            response.status = 400; // 客户端参数缺失
            response.set_content(errorJsonStr, "application/json");
            return;
        }

        // 3. 调用底层的 ChatSDK 发送消息
        // SDK 内部会自动完成：查找会话 -> 获取历史上下文 -> 组合参数 -> 请求大模型 API -> 存入数据库
        std::string assistantMessage = _chatSDK->sendMessage(sessionId, message);

        if (assistantMessage.empty())
        {
            // 如果返回为空，说明大模型请求失败或网络超时
            std::string errorJsonStr = buildResponse("Failed to send AI response message (Internal Error)", false);
            response.status = 500; // 500 服务器内部错误
            response.set_content(errorJsonStr, "application/json");
            return;
        }

        // 4. 构造成功的响应体
        Json::Value dataJson;
        dataJson["session_id"] = sessionId;
        dataJson["response"] = assistantMessage; // 存放 AI 的完整回复

        Json::Value responseJson;
        responseJson["success"] = true;
        responseJson["message"] = "send message success";
        responseJson["data"] = dataJson;

        // 5. 序列化并填充 HTTP 响应
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = ""; // 使用紧凑格式
        std::string responseJsonStr = Json::writeString(writerBuilder, responseJson);

        response.status = 200; // HTTP 200 OK
        response.set_content(responseJsonStr, "application/json");
    }

    // 处理发送消息请求 - 增量返回 (流式响应)
    void ChatServer::handleSendMessageStreamRequest(const httplib::Request &request, httplib::Response &response)
    {
        // 1. 获取并解析请求参数
        Json::Value requestJson;
        Json::Reader reader;
        if (!reader.parse(request.body, requestJson))
        {
            std::string errorJsonStr = buildResponse("parse request body failed, json format error", false);
            response.status = 400;
            response.set_content(errorJsonStr, "application/json");
            return;
        }

        std::string sessionId = requestJson["session_id"].asString();
        std::string message = requestJson["message"].asString();

        if (sessionId.empty() || message.empty())
        {
            std::string errorJsonStr = buildResponse("session_id or message is empty", false);
            response.status = 400;
            response.set_content(errorJsonStr, "application/json");
            return;
        }

        // 2. 准备流式响应的 HTTP 头部
        response.status = 200;
        response.set_header("Cache-Control", "no-cache");        // 禁用缓存
        response.set_header("Connection", "keep-alive");         // 保持长连接
        response.set_header("Access-Control-Allow-Origin", "*"); // 允许跨域请求
        response.set_header("Access-Control-Allow-Headers", "*");

        // 3. 设置分块内容提供者 (Chunked Content Provider)
        // "text/event-stream" 告诉前端我们将使用 SSE 协议传输
        response.set_chunked_content_provider("text/event-stream",
                                              [this, sessionId, message](size_t offset, httplib::DataSink &dataSink) -> bool
                                              {
                                                  // 定义内部的回调函数，用于接收底层 ChatSDK 吐出的增量数据
                                                  auto writeChunk = [&](const std::string &chunk, bool last)
                                                  {
                                                      // 将纯文本 chunk 转换为合法的 JSON 字符串，防止特殊字符（特别是 \n）破坏 SSE 协议结构
                                                      std::string sseData = "data: " + Json::valueToQuotedString(chunk.c_str()) + "\n\n";

                                                      // 使用 DataSink 立刻将数据写入网络套接字，推送到客户端
                                                      dataSink.write(sseData.c_str(), sseData.size());

                                                      // 处理结束标记
                                                      if (last)
                                                      {
                                                          std::string doneData = "data: [DONE]\n\n";
                                                          dataSink.write(doneData.c_str(), doneData.size());
                                                          dataSink.done(); // 通知 HTTP 引擎当前流式响应已彻底结束
                                                          return false;    // 终止底层等待
                                                      }
                                                      return true;
                                                  };

                                                  // 【细节优化】：在发起可能耗时较长的大模型请求前，
                                                  // 先给客户端发送一个空的数据块，确保 HTTP 响应头立刻发出去，避免前端因首包超时而断开连接。
                                                  if (!writeChunk("", false))
                                                  {
                                                      return false;
                                                  }

                                                  // 4. 调用底层 SDK 发送流式请求
                                                  // 该调用会阻塞当前工作线程，但内部会不断触发 writeChunk 回调
                                                  _chatSDK->sendMessageStream(sessionId, message, writeChunk);

                                                  return false; // 返回 false 表示所有数据提供完毕，关闭连接
                                              });
    }

    // 设置 HTTP 路由规则
    void ChatServer::setHttpRoutes()
    {
        // 1. 处理创建会话请求
        // 接口：POST /api/session
        _chatServer->Post("/api/session", [this](const httplib::Request &request, httplib::Response &response)
                          { handleCreateSessionRequest(request, response); });

        // 2. 处理获取会话列表请求
        // 接口：GET /api/sessions
        _chatServer->Get("/api/sessions", [this](const httplib::Request &request, httplib::Response &response)
                         { handleGetSessionListsRequest(request, response); });

        // 3. 处理获取模型列表请求
        // 接口：GET /api/models
        _chatServer->Get("/api/models", [this](const httplib::Request &request, httplib::Response &response)
                         { handleGetModelListsRequest(request, response); });

        // 4. 处理删除会话请求 (带路径参数)
        // 接口：DELETE /api/session/{sessionId}
        // 使用正则表达式 (.*) 捕获任意字符组合作为 sessionId
        _chatServer->Delete("/api/session/(.*)", [this](const httplib::Request &request, httplib::Response &response)
                            { handleDeleteSessionRequest(request, response); });

        // 5. 处理获取历史消息请求 (带路径参数)
        // 接口：GET /api/session/{sessionId}/history
        _chatServer->Get("/api/session/(.*)/history", [this](const httplib::Request &request, httplib::Response &response)
                         { handleGetHistoryMessagesRequest(request, response); });

        // 6. 处理发送消息请求 - 全量返回
        // 接口：POST /api/message
        _chatServer->Post("/api/message", [this](const httplib::Request &request, httplib::Response &response)
                          { handleSendMessageRequest(request, response); });

        // 7. 处理发送消息请求 - 增量返回 (流式 SSE)
        // 接口：POST /api/message/async
        _chatServer->Post("/api/message/async", [this](const httplib::Request &request, httplib::Response &response)
                          { handleSendMessageStreamRequest(request, response); });
    }

} // namespace ai_chat_server