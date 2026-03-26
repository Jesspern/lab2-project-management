#include "AuthHandler.h"
#include "../models/User.h"
#include "../models/Role.h"
#include "../models/dto/LoginRequest.h"
#include "../models/dto/LoginResponse.h"
#include "../models/dto/ErrorResponse.h"
#include "../utils/JsonHelper.h"
#include "../storage/Storage.h"
#include <Poco/Logger.h>
#include <Poco/Timestamp.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JWT/Token.h>
#include <Poco/JWT/Signer.h>
#include <Poco/Environment.h>
#include <sstream>

namespace handlers {

void AuthHandler::handleRequest(Poco::Net::HTTPServerRequest& request,
                                 Poco::Net::HTTPServerResponse& response) {
    Poco::Timestamp start;
    
    response.setContentType("application/json");
    response.set("Access-Control-Allow-Origin", "*");
    
    const std::string& method = request.getMethod();
    const std::string& path = request.getURI();
    
    auto& logger = Poco::Logger::get("ProjectAPI");
    logger.information("%s %s", method, path);
    
    // POST /api/auth — Логин
    if (path == "/api/auth" && method == "POST") {
        try {
            // Парсим запрос через DTO
            Poco::JSON::Parser parser;
            auto json = parser.parse(request.stream()).extract<Poco::JSON::Object::Ptr>();

            dto::LoginRequest req = dto::LoginRequest::fromJson(json);
            
            // Валидация обязательных полей
            if (!req.isValid()) {
                response.setStatus(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
                dto::ErrorResponse::create("validation_error", "login and password required", 400)
                    ->stringify(response.send());
                logger.warning("400 POST /api/auth - validation failed");
                return;
            }
            
            std::string login = req.login;
            std::string password = req.password;
            
            // Проверяем пользователя
            auto it = storage::loginToId.find(login);
            if (it == storage::loginToId.end()) {
                response.setStatus(Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED);
                dto::ErrorResponse::create("invalid_credentials", "User not found", 401)
                    ->stringify(response.send());
                logger.warning("401 POST /api/auth - User %s not found", login);
                return;
            }
            
            const auto& user = storage::users[it->second];

            logger.information("DEBUG: Login - user found: id=%d, login=%s, role_in_storage=%s", 
                  user.id,
                  user.login,
                  models::roleToString(user.role));
                  
            if (user.password != password) {
                response.setStatus(Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED);
                dto::ErrorResponse::create("invalid_credentials", "Wrong password", 401)
                    ->stringify(response.send());
                logger.warning("401 POST /api/auth - Wrong password for %s", login);
                return;
            }

            logger.information("DEBUG: JWT created - payload role=%s", 
                  models::roleToString(user.role));
            
            // Создаём JWT токен с ролью в payload
            std::string secret = Poco::Environment::get("JWT_SECRET", "dev-secret-key");
            if (secret.empty()) {
                response.setStatus(Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
                dto::ErrorResponse::create("config_error", "JWT_SECRET not configured", 500)
                    ->stringify(response.send());
                logger.error("500 POST /api/auth - JWT_SECRET not configured");
                return;
            }
            
            Poco::JWT::Token token;
            token.setType("JWT");
            token.setSubject(user.login);
            token.setIssuedAt(Poco::Timestamp());
            token.payload().set("user_id", user.id);
            token.payload().set("role", models::roleToString(user.role));
            
            Poco::JWT::Signer signer(secret);
            std::string jwt = signer.sign(token, Poco::JWT::Signer::ALGO_HS256);
            
            // Формируем ответ через DTO
            response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
            dto::LoginResponse resp;
            resp.token = jwt;
            resp.userId = user.id;
            resp.login = user.login;
            resp.role = models::roleToString(user.role);
            resp.toJson()->stringify(response.send());
            
            logger.information("200 POST /api/auth - User %s (%s) logged in", 
                            user.login, models::roleToString(user.role));
            
        } catch (const Poco::Exception& e) {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
            dto::ErrorResponse::create("invalid_json", "Invalid JSON format", 400)
                ->stringify(response.send());
            logger.warning("400 POST /api/auth - Invalid JSON: %s", e.message());
        } catch (const std::exception& e) {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
            dto::ErrorResponse::create("internal_error", "Internal error", 500)
                ->stringify(response.send());
            logger.error("500 POST /api/auth - %s", e.what());
        }
    }
    else {
        response.setStatus(Poco::Net::HTTPResponse::HTTP_METHOD_NOT_ALLOWED);
        dto::ErrorResponse::create("method_not_allowed", "Only POST is supported", 405)
            ->stringify(response.send());
    }
}

}