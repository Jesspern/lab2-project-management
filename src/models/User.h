#pragma once
#include <string>
#include "Role.h"

namespace models {

struct User {
    int id;
    std::string login;
    std::string password;
    std::string firstName;
    std::string lastName;
    std::string email;
    Role role;
    
    User() : id(0) {}
    
    User(int id, const std::string& login, const std::string& password,
         const std::string& firstName, const std::string& lastName,
         const std::string& email, Role role)
        : id(id), login(login), password(password),
          firstName(firstName), lastName(lastName), email(email), role(role) {}
};

}