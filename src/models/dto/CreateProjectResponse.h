#pragma once
#include <string>
#include <Poco/JSON/Object.h>
#include "../Project.h"

namespace dto {

struct CreateProjectResponse {
    int id;
    std::string name;
    std::string description;
    std::string code;
    int ownerId;
    
    static CreateProjectResponse fromProject(const models::Project& project) {
        CreateProjectResponse resp;
        resp.id = project.id;
        resp.name = project.name;
        resp.description = project.description;
        resp.code = project.code;
        resp.ownerId = project.ownerId;
        return resp;
    }
    
    Poco::JSON::Object::Ptr toJson() const {
        Poco::JSON::Object::Ptr json = new Poco::JSON::Object();
        json->set("id", id);
        json->set("name", name);
        json->set("description", description);
        json->set("code", code);
        json->set("ownerId", ownerId);
        return json;
    }
};

}