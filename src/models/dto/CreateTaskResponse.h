#pragma once
#include <string>
#include <Poco/JSON/Object.h>
#include "../Task.h"

namespace dto {

struct CreateTaskResponse {
    int id;
    std::string code;
    std::string title;
    std::string status;
    int projectId;
    int assigneeId;
    
    static CreateTaskResponse fromTask(const models::Task& task) {
        CreateTaskResponse resp;
        resp.id = task.id;
        resp.code = task.code;
        resp.title = task.title;
        resp.status = models::taskStatusToString(task.status);
        resp.projectId = task.projectId;
        resp.assigneeId = task.assigneeId;
        return resp;
    }
    
    Poco::JSON::Object::Ptr toJson() const {
        Poco::JSON::Object::Ptr json = new Poco::JSON::Object();
        json->set("id", id);
        json->set("code", code);
        json->set("title", title);
        json->set("status", status);
        json->set("projectId", projectId);
        json->set("assigneeId", assigneeId);
        return json;
    }
};

}