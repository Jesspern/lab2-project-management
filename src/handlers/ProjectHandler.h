#pragma once
#include <Poco/Net/HTTPRequestHandler.h>

namespace handlers {

class ProjectHandler : public Poco::Net::HTTPRequestHandler {
public:
    void handleRequest(Poco::Net::HTTPServerRequest& request,
                       Poco::Net::HTTPServerResponse& response) override;
};

}