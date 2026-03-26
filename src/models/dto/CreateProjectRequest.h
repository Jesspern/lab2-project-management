#pragma once
#include <string>
#include <Poco/JSON/Object.h>
#include "../../utils/JsonHelper.h"

namespace dto {

struct CreateProjectRequest {
    std::string name;
    std::string description;
    
    bool isValid() const {
        return !name.empty();
    }
    
    static CreateProjectRequest fromJson(const Poco::JSON::Object::Ptr& json) {
        CreateProjectRequest req;
        req.name = utils::getRequired<std::string>(json, "name");
        req.description = utils::getString(json, "description", "");
        return req;
    }
};

}