#pragma once
#include <string>
#include <Poco/JSON/Object.h>
#include "../User.h"

namespace dto {

struct CreateUserResponse {
    int id;
    std::string login;
    std::string firstName;
    std::string lastName;
    std::string email;
    std::string role;
    
    // Конвертация в JSON для ответа
    Poco::JSON::Object::Ptr toJson() const {
        Poco::JSON::Object::Ptr json = new Poco::JSON::Object();
        json->set("id", id);
        json->set("login", login);
        json->set("firstName", firstName);
        json->set("lastName", lastName);
        json->set("email", email);
        json->set("role", role);
        return json;
    }
    
    // Фабричный метод из модели
    static CreateUserResponse fromUser(const models::User& user) {
        CreateUserResponse resp;
        resp.id = user.id;
        resp.login = user.login;
        resp.firstName = user.firstName;
        resp.lastName = user.lastName;
        resp.email = user.email;
        resp.role = models::roleToString(user.role);
        return resp;
    }
};

}