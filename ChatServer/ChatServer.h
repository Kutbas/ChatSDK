#pragma once
#include <httplib.h>
#include <memory>
#include <string>
#include <atomic>
// 引入我们之前封装好的顶层 SDK 头文件
#include <ai_chat_sdk/ChatSDK.h>

namespace ai_chat_server
{

    // 服务器与模型的基础配置信息
    struct ServerConfig
    {
        // 1. HTTP 服务器网络配置
        std::string host = "0.0.0.0";  // 绑定所有网卡，允许外部访问
        int port = 8080;               // 监听端口
        std::string logLevel = "INFO"; // 日志打印级别

        // 2. 模型公共调节参数
        double temperature = 0.7; // 控制生成的随机性
        int maxTokens = 1024;     // 最大生成的 token 数量

        // 3. 云端模型 API Key
        std::string deepseekAPIKey;
        std::string geminiAPIKey;
        std::string chatGPTAPIKey;

        // 4. Ollama 本地模型配置
        std::string ollamaModelName; // 例如: deepseek-r1:1.5b
        std::string ollamaModelDesc;
        std::string ollamaEndpoint; // 例如: http://127.0.0.1:11434
    };

    class ChatServer
    {
    public:
        // 构造函数：传入配置并完成初始化
        ChatServer(const ServerConfig &config);

        bool start();           // 启动服务器 (阻塞)
        void stop();            // 停止服务器
        bool isRunning() const; // 检查运行状态

    private:
        // 辅助工具：统一构建 JSON 格式的 HTTP 响应体
        std::string buildResponse(const std::string &message, bool success = false);

        // ================= HTTP 路由处理函数 =================
        void handleCreateSessionRequest(const httplib::Request &request, httplib::Response &response);
        void handleGetSessionListsRequest(const httplib::Request &request, httplib::Response &response);
        void handleGetModelListsRequest(const httplib::Request &request, httplib::Response &response);
        void handleDeleteSessionRequest(const httplib::Request &request, httplib::Response &response);
        void handleGetHistoryMessagesRequest(const httplib::Request &request, httplib::Response &response);

        // 核心聊天接口
        void handleSendMessageRequest(const httplib::Request &request, httplib::Response &response);       // 全量
        void handleSendMessageStreamRequest(const httplib::Request &request, httplib::Response &response); // 流式 SSE

        // 注册所有的 HTTP 路由规则
        void setHttpRoutes();

    private:
        ServerConfig _config; // 保存传入的配置

        // HTTP 服务器底层对象 (独占所有权)
        std::unique_ptr<httplib::Server> _chatServer = nullptr;

        // 我们亲手打造的 ChatSDK (共享所有权，方便后续可能的多线程访问)
        std::shared_ptr<ai_chat_sdk::ChatSDK> _chatSDK = nullptr;

        // 服务器运行状态标记，使用原子变量保证线程安全
        std::atomic<bool> _isRunning = {false};
    };

} // namespace ai_chat_server