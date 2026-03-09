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
        geminiConfig->_modelName = "gemini-2.0-flash";
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

} // namespace ai_chat_server