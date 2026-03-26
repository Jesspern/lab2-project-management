#pragma once
#include <string>
#include <Poco/JSON/Object.h>
#include "../../utils/JsonHelper.h"
#include "../Role.h"

namespace dto {

struct CreateUserRequest {
    std::string login;
    std::string password;
    std::string firstName;
    std::string lastName;
    std::string email;
    models::Role role;
    
    // Валидация
    bool isValid() const {
        return !login.empty() && !password.empty() && !firstName.empty();
    }
    
    // Парсинг из JSON (POCO)
    static CreateUserRequest fromJson(const Poco::JSON::Object::Ptr& json) {
        CreateUserRequest req;
        req.login = json->getValue<std::string>("login");
        req.password = json->getValue<std::string>("password");
        req.firstName = json->getValue<std::string>("firstName");
        req.lastName = json->getValue<std::string>("lastName");
        req.email = json->getValue<std::string>("email");

        std::string roleStr = utils::getString(json, "role", "WORKER");
        req.role = models::stringToRole(roleStr);

        return req;
    }
};

}