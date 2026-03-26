#pragma once
#include <string>
#include <Poco/JSON/Object.h>
#include "../../utils/JsonHelper.h"

namespace dto {

struct LoginRequest {
    std::string login;
    std::string password;
    
    bool isValid() const {
        return !login.empty() && !password.empty();
    }
    
    // Метод парсинга из JSON
    static LoginRequest fromJson(const Poco::JSON::Object::Ptr& json) {
        LoginRequest req;
        
        // Обязательные поля
        req.login = utils::getRequired<std::string>(json, "login");
        req.password = utils::getRequired<std::string>(json, "password");
        
        return req;
    }
};

}