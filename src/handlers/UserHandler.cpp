#include "UserHandler.h"
#include "../models/User.h"
#include "../models/Role.h"
#include "../models/dto/CreateUserRequest.h"
#include "../models/dto/CreateUserResponse.h"
#include "../models/dto/ErrorResponse.h"
#include "../utils/JsonHelper.h"
#include "../database/MongoDBManager.h"
#include "middleware/AuthMiddleware.h"
#include <Poco/Logger.h>
#include <Poco/Timestamp.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Stringifier.h>
#include <Poco/URI.h>
#include <Poco/Exception.h>
#include <sstream>

namespace handlers {

void UserHandler::handleRequest(Poco::Net::HTTPServerRequest& request,
                                 Poco::Net::HTTPServerResponse& response) {
    Poco::Timestamp start;
    
    response.setContentType("application/json");
    response.set("Access-Control-Allow-Origin", "*");
    
    const std::string& method = request.getMethod();
    Poco::URI uri(request.getURI());
    std::string path = uri.getPath();
    
    auto& logger = Poco::Logger::get("ProjectAPI");
    logger.information("%s %s", method, path);
    
    // POST /api/users
    if (path == "/api/users" && method == "POST") {
        try {
            // Парсим запрос через DTO
            Poco::JSON::Parser parser;
            auto json = parser.parse(request.stream()).extract<Poco::JSON::Object::Ptr>();

            dto::CreateUserRequest req = dto::CreateUserRequest::fromJson(json);
            
            // Валидация обязательных полей
            if (!req.isValid()) {
                response.setStatus(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
                dto::ErrorResponse::create("validation_error", "login, password and firstName required", 400)
                    ->stringify(response.send());
                logger.warning("400 POST /api/users - validation failed");
                return;
            }
            
            std::string login = utils::getString(json, "login");

            logger.information("DEBUG: Register request - login=%s, role_str=%s, role_enum=%d", 
                  req.login, 
                  utils::getString(json, "role", "NOT_SET"),
                  static_cast<int>(req.role));
            
            // Проверка на дубликат
           if (database::MongoDBManager::instance().getUserByLogin(req.login).has_value()) {
                response.setStatus(Poco::Net::HTTPResponse::HTTP_CONFLICT);
                dto::ErrorResponse::create("conflict", "User already exists", 409)
                    ->stringify(response.send());
                logger.warning("409 POST /api/users - User %s already exists", req.login);
                return;
            }
            
            // Создаём пользователя
            models::User user;
            user.login = req.login;
            user.password = req.password;
            user.firstName = req.firstName;
            user.lastName = req.lastName;
            user.email = req.email;
            user.role = req.role;  
            
            auto created = database::MongoDBManager::instance().createUser(user);
            if (!created.has_value()) {
                response.setStatus(Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
                dto::ErrorResponse::create("db_error", "Failed to create user", 500)
                    ->stringify(response.send());
                return;
            }
            user = created.value();

            logger.information("DEBUG: User saved - id=%d, login=%s, role=%s", 
                  user.id, 
                  user.login, 
                  models::roleToString(user.role));
            
            // Ответ через DTO
            response.setStatus(Poco::Net::HTTPResponse::HTTP_CREATED);
            dto::CreateUserResponse resp = dto::CreateUserResponse::fromUser(user);
            resp.toJson()->stringify(response.send());
            
            logger.information("201 POST /api/users - User %d (%s) created with role %s", 
                             user.id, user.login, models::roleToString(user.role));
            
        } catch (const Poco::Exception& e) {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
            dto::ErrorResponse::create("invalid_json", "Invalid JSON format", 400)
                ->stringify(response.send());
            logger.warning("400 POST /api/users - Invalid JSON: %s", e.message());
        } catch (const std::exception& e) {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
            dto::ErrorResponse::create("internal_error", "Internal error", 500)
                ->stringify(response.send());
            logger.error("500 POST /api/users - %s", e.what());
        }
    }
    
    // GET /api/users/login/{login} 
    else if (path.find("/api/users/login/") == 0 && method == "GET") {
        std::string login = path.substr(17);  // "/api/users/login/".length() = 17
        
        auto userOpt = database::MongoDBManager::instance().getUserByLogin(login);
        if (userOpt.has_value()) {
            const auto& user = userOpt.value();
            response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
            
            Poco::JSON::Object::Ptr json = new Poco::JSON::Object();
            json->set("id", user.id);
            json->set("login", user.login);
            json->set("firstName", user.firstName);
            json->set("lastName", user.lastName);
            json->set("email", user.email);
            json->set("role", models::roleToString(user.role));
            json->stringify(response.send());
            
            logger.information("200 GET /api/users/login/%s", login);
        } else {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
            dto::ErrorResponse::create("not_found", "User not found", 404)
                ->stringify(response.send());
            logger.warning("404 GET /api/users/login/%s - not found", login);
        }
    }
    
    // GET /api/users/search?firstName=&lastName=
    else if (path == "/api/users/search" && method == "GET") {
        // Проверяем авторизацию
        auto auth = middleware::AuthMiddleware::verifyToken(request);
        if (!auth.authenticated) {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED);
            dto::ErrorResponse::create("unauthorized", "Login required", 401)
                ->stringify(response.send());
            logger.warning("401 GET /api/users/search - not authenticated");
            return;
        }
        
        // Проверяем роль
        if (!middleware::AuthMiddleware::hasPermission(auth, "user:read")) {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_FORBIDDEN);
            dto::ErrorResponse::create("forbidden", "Insufficient permissions", 403)
                ->stringify(response.send());
            logger.warning("403 GET /api/users/search - User %s denied", auth.login);
            return;
        }
        
        // Выполняем поиск
        auto params = uri.getQueryParameters();
        std::string firstName = utils::getQueryParam(uri, "firstName", "");
        std::string lastName = utils::getQueryParam(uri, "lastName", "");
        
        Poco::JSON::Array::Ptr results = new Poco::JSON::Array();
        auto users = database::MongoDBManager::instance().searchUsers(firstName, lastName);
        for (const auto& user : users) {
            Poco::JSON::Object::Ptr json = new Poco::JSON::Object();
            json->set("id", user.id);
            json->set("login", user.login);
            json->set("firstName", user.firstName);
            json->set("lastName", user.lastName);
            json->set("role", models::roleToString(user.role));
            results->add(json);
        }
        
        response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
        Poco::JSON::Object::Ptr result = new Poco::JSON::Object();
        result->set("users", results);
        result->stringify(response.send());
        
        logger.information("200 GET /api/users/search - found %zu users", results->size());
    }
    
    // DELETE /api/users/{id} 
    else if (path.find("/api/users/") == 0 && method == "DELETE") {
        // Проверяем авторизацию
        auto auth = middleware::AuthMiddleware::verifyToken(request);
        if (!auth.authenticated) {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED);
            dto::ErrorResponse::create("unauthorized", "Login required", 401)
                ->stringify(response.send());
            return;
        }
        
        // Проверяем роль
        if (!middleware::AuthMiddleware::hasPermission(auth, "user:delete")) {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_FORBIDDEN);
            dto::ErrorResponse::create("forbidden", "Administrator access required", 403)
                ->stringify(response.send());
            logger.warning("403 DELETE /api/users - User %s denied", auth.login);
            return;
        }
        
        // Извлекаем ID и удаляем
        try {
            int userId = std::stoi(path.substr(13));  // "/api/users/".length() = 13
            
            if (!database::MongoDBManager::instance().deleteUser(userId)) {
                response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
                dto::ErrorResponse::create("not_found", "User not found", 404)
                    ->stringify(response.send());
                return;
            }
            
            response.setStatus(Poco::Net::HTTPResponse::HTTP_NO_CONTENT);
            logger.information("204 DELETE /api/users/%d - deleted by %s",
                             userId, auth.login.c_str());
            
        } catch (const std::exception& e) {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
            dto::ErrorResponse::create("invalid_id", "Invalid user ID", 400)
                ->stringify(response.send());
        }
    }
    
    // Неизвестный эндпоинт
    else {
        response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
        dto::ErrorResponse::create("not_found", "Endpoint not found", 404)
            ->stringify(response.send());
    }
}

}