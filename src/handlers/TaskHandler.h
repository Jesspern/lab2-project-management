#pragma once
#include <Poco/Net/HTTPRequestHandler.h>

namespace handlers {

class TaskHandler : public Poco::Net::HTTPRequestHandler {
public:
    void handleRequest(Poco::Net::HTTPServerRequest& request,
                       Poco::Net::HTTPServerResponse& response) override;
};

}