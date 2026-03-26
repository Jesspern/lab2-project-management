#pragma once
#include <string>
#include <Poco/JSON/Object.h>

namespace dto {

struct LoginResponse {
    std::string token;
    int userId;
    std::string login;
    std::string role;
    
    Poco::JSON::Object::Ptr toJson() const {
        Poco::JSON::Object::Ptr json = new Poco::JSON::Object();
        json->set("token", token);
        json->set("user_id", userId);
        json->set("login", login);
        json->set("role", role);
        return json;
    }
};

}