#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Logger.h>
#include <Poco/ConsoleChannel.h>
#include <Poco/AutoPtr.h>
#include <Poco/Environment.h>
#include <thread>
#include <chrono>

#include "handlers/RouterFactory.h"

using namespace Poco::Net;
using namespace Poco;

int main(int argc, char** argv) {
    // Настройка логгера
    AutoPtr<ConsoleChannel> consoleChannel(new ConsoleChannel);
    Logger::root().setChannel(consoleChannel);
    Logger::root().setLevel(Message::PRIO_INFORMATION);
    
    auto& logger = Logger::get("ProjectAPI");
    logger.information("Starting Project Management API");
    
    // Порт из переменной окружения
    unsigned short port = 8080;
    if (Environment::has("PORT")) {
        port = static_cast<unsigned short>(std::stoi(Environment::get("PORT")));
    }
    
    ServerSocket socket(port);
    HTTPServerParams::Ptr params = new HTTPServerParams;
    params->setMaxThreads(16);
    params->setMaxQueued(100);
    
    HTTPServer server(new handlers::RouterFactory(), socket, params);
    server.start();
    
    logger.information("Server started on http://localhost:%hu", port);
    logger.information("Endpoints:");
    logger.information("  POST   /api/users          - Register user");
    logger.information("  GET    /api/users/login/{login} - Get user by login");
    logger.information("  GET    /api/users/search   - Search users");
    logger.information("  POST   /api/auth           - Login (get JWT token)");
    logger.information("  GET    /api/health         - Health check");
    
    // Бесконечный цикл для Docker
    while (true) {
        std::this_thread::sleep_for(std::chrono::hours(1));
    }
    
    return 0;
}