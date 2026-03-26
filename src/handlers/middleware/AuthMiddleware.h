#pragma once
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/JWT/Token.h>
#include <Poco/JWT/Signer.h>
#include <Poco/Environment.h>
#include "../../models/Role.h"
#include "../../utils/JsonHelper.h"

namespace middleware {

struct AuthResult {
    bool authenticated;
    int userId;
    std::string login;
    models::Role role;
    std::string errorMessage;
    
    AuthResult() : authenticated(false), userId(0), role(models::Role::GUEST) {}
};

class AuthMiddleware {
public:
    static AuthResult verifyToken(Poco::Net::HTTPServerRequest& request) {
        AuthResult result;
        
        // Получаем заголовок
        std::string authHeader = request.get("Authorization", "");
        if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
            result.errorMessage = "Bearer token required";
            return result;
        }
        
        // Извлекаем токен
        std::string jwt = authHeader.substr(7);
        
        // Получаем секрет из ENV
        std::string secret = Poco::Environment::get("JWT_SECRET", "dev-secret-key");
        if (secret.empty()) {
            result.errorMessage = "JWT_SECRET not configured";
            return result;
        }
        
        try {
            // Верифицируем токен
            Poco::JWT::Signer signer(secret);
            Poco::JWT::Token token = signer.verify(jwt);
            
            // Извлекаем данные из payload
            auto& payload = token.payload();
            
            result.authenticated = true;
            result.userId = utils::getInt(payload, "user_id", 0);
            result.login = token.getSubject();
            result.role = models::stringToRole(
                utils::getString(payload, "role", "GUEST")
            );
            
            return result;
            
        } catch (const Poco::SyntaxException& e) {
            // Невалидный формат JWT
            result.errorMessage = "Invalid token format";
            return result;
        } catch (const Poco::InvalidArgumentException& e) {
            // Неверный секрет или алгоритм
            result.errorMessage = "Invalid token signature";
            return result;
        } catch (const Poco::NotFoundException& e) {
            // Поле не найдено в payload
            result.errorMessage = "Token missing required field";
            return result;
        } catch (const Poco::Exception& e) {
            result.errorMessage = "Token verification failed";
            return result;
        } catch (const std::exception& e) {
            // Стандартные исключения C++
            result.errorMessage = "Unexpected error";
            return result;
        }
    }
    
    static bool hasPermission(const AuthResult& auth, const std::string& permission) {
        if (!auth.authenticated) return false;
        return models::hasPermission(auth.role, permission);
    }
};

}