#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <ai_chat_sdk/ChatSDK.h>
#include <ai_chat_sdk/util/myLog.h>

void sendMessageStream(ai_chat_sdk::ChatSDK &chatSDK, const std::string &sessionId)
{
    std::cout << "\n-------------- 发送消息 --------------\nUser > ";
    std::string message;
    std::getline(std::cin, message);
    if (message.empty())
        return;

    std::cout << "--------------------------------------\nAssistant > ";
    // 使用 Lambda 表达式处理流式响应
    chatSDK.sendMessageStream(sessionId, message, [](const std::string &chunk, bool done)
                              {
        std::cout << chunk << std::flush;
        if (done) std::cout << "\n-------------- 回复结束 --------------\n"; });
}

int main()
{
    // 1. 初始化日志系统 (必须步骤)
    mylog::Logger::initLogger("aiChatServer", "stdout", spdlog::level::info);

    // 2. 配置 DeepSeek 模型
    ai_chat_sdk::ChatSDK chatSDK;
    ai_chat_sdk::APIConfig deepseekConfig;

    // 建议从环境变量获取 API Key 保证安全
    const char *apiKey = std::getenv("deepseek_apikey");
    if (!apiKey)
    {
        std::cerr << "Error: 请设置环境变量 deepseek_apikey" << std::endl;
        return -1;
    }

    deepseekConfig._apiKey = apiKey;
    deepseekConfig._temperature = 0.7;
    deepseekConfig._maxTokens = 2048;
    deepseekConfig._modelName = "deepseek-chat";

    std::vector<std::shared_ptr<ai_chat_sdk::Config>> configs;
    configs.push_back(std::make_shared<ai_chat_sdk::APIConfig>(deepseekConfig));

    // 3. 初始化 SDK 并创建会话
    chatSDK.initModels(configs);
    std::string sessionId = chatSDK.createSession("deepseek-chat");
    std::cout << "会话创建成功，Session ID: " << sessionId << std::endl;

    // 4. 开启交互循环
    int userOp = 1;
    while (true)
    {
        std::cout << "\n[1] 发送消息 [0] 退出程序: ";
        if (!(std::cin >> userOp) || userOp == 0)
            break;
        std::cin.ignore(); // 清理换行符
        sendMessageStream(chatSDK, sessionId);
    }
    std::cout << "程序退出..." << std::endl;
    return 0;
}