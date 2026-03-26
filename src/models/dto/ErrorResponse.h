#pragma once
#include <string>
#include <Poco/JSON/Object.h>

namespace dto {

struct ErrorResponse {
    std::string error;
    std::string message;
    int code;
    
    Poco::JSON::Object::Ptr toJson() const {
        Poco::JSON::Object::Ptr json = new Poco::JSON::Object();
        json->set("error", error);
        if (!message.empty()) {
            json->set("message", message);
        }
        json->set("code", code);
        return json;
    }
    
    static Poco::JSON::Object::Ptr create(const std::string& error, 
                                          const std::string& message = "",
                                          int code = 400) {
        ErrorResponse resp{error, message, code};
        return resp.toJson();
    }
};

}