#pragma once
#include <string>
#include <Poco/JSON/Object.h>
#include "../../utils/JsonHelper.h"

namespace dto {

struct CreateTaskRequest {
    std::string title;
    std::string description;
    int assigneeId;
    
    bool isValid() const {
        return !title.empty();
    }
    
    static CreateTaskRequest fromJson(const Poco::JSON::Object::Ptr& json) {
        CreateTaskRequest req;
        req.title = utils::getRequired<std::string>(json, "title");
        req.description = utils::getString(json, "description", "");
        req.assigneeId = utils::getInt(json, "assigneeId", 0);
        return req;
    }
};

}