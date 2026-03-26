#include "RouterFactory.h"
#include "UserHandler.h"
#include "AuthHandler.h"
#include "ProjectHandler.h"
#include "TaskHandler.h"

namespace handlers {

Poco::Net::HTTPRequestHandler* RouterFactory::createRequestHandler(
    const Poco::Net::HTTPServerRequest& request) {
    
    const std::string& uri = request.getURI();
    const std::string& method = request.getMethod();
    
    // Auth endpoints
    if (uri == "/api/auth" && method == "POST") {
        return new AuthHandler();
    }
    
    // User endpoints 
    if (uri.find("/api/users") == 0) {
        return new UserHandler();
    }

    // Project endpoints
    if (uri.find("/api/projects") == 0 && uri.find("/tasks") == std::string::npos) {
        return new ProjectHandler();
    }
    
    // Task endpoints
    if (uri.find("/api/projects/") == 0 && uri.find("/tasks") != std::string::npos ||
        uri.find("/api/tasks/") == 0) {
        return new TaskHandler();
    }
    
    // Health check
    if (uri == "/api/health" || uri == "/") {
        class HealthHandler : public Poco::Net::HTTPRequestHandler {
        public:
            void handleRequest(Poco::Net::HTTPServerRequest& request,
                               Poco::Net::HTTPServerResponse& response) override {
                response.setContentType("application/json");
                response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                response.send() << "{\"status\": \"ok\", \"message\": \"Server is running\"}";
            }
        };
        return new HealthHandler();
    }
    
    // 404
    class NotFoundHandler : public Poco::Net::HTTPRequestHandler {
    public:
        void handleRequest(Poco::Net::HTTPServerRequest& request,
                           Poco::Net::HTTPServerResponse& response) override {
            response.setContentType("application/json");
            response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
            response.send() << "{\"error\": \"Endpoint not found\"}";
        }
    };
    return new NotFoundHandler();
}

}