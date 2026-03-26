#include "ProjectHandler.h"
#include "../models/Project.h"
#include "../models/Role.h"
#include "../models/dto/CreateProjectRequest.h"
#include "../models/dto/CreateProjectResponse.h"
#include "../models/dto/ErrorResponse.h"
#include "../utils/JsonHelper.h"
#include "../handlers/middleware/AuthMiddleware.h"
#include "ProjectHandler.h"
#include "../models/Project.h"
#include "../storage/Storage.h"
#include <Poco/Logger.h>
#include <Poco/Timestamp.h>
#include <Poco/JSON/Parser.h>
#include <Poco/URI.h>
#include <map>
#include <sstream>

namespace handlers {

void ProjectHandler::handleRequest(Poco::Net::HTTPServerRequest& request,
                                    Poco::Net::HTTPServerResponse& response) {
    response.setContentType("application/json");
    response.set("Access-Control-Allow-Origin", "*");
    
    const std::string& method = request.getMethod();
    Poco::URI uri(request.getURI());
    std::string path = uri.getPath();
    
    auto& logger = Poco::Logger::get("ProjectAPI");
    logger.information("%s %s", method, path);
    
    // POST /api/projects (TRACKER, ADMIN)
    if (path == "/api/projects" && method == "POST") {
        // Проверка авторизации
        auto auth = middleware::AuthMiddleware::verifyToken(request);
        if (!auth.authenticated) {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED);
            dto::ErrorResponse::create("unauthorized", "Login required", 401)
                ->stringify(response.send());
            return;
        }
        
        // Проверка прав (project:write)
        if (!middleware::AuthMiddleware::hasPermission(auth, "project:write")) {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_FORBIDDEN);
            dto::ErrorResponse::create("forbidden", "TRACKER or ADMIN role required", 403)
                ->stringify(response.send());
            logger.warning("403 POST /api/projects - User %s denied", auth.login);
            return;
        }
        
        try {
            // Парсим запрос
            Poco::JSON::Parser parser;
            auto json = parser.parse(request.stream()).extract<Poco::JSON::Object::Ptr>();
            dto::CreateProjectRequest req = dto::CreateProjectRequest::fromJson(json);
            
            if (!req.isValid()) {
                response.setStatus(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
                dto::ErrorResponse::create("validation_error", "name is required", 400)
                    ->stringify(response.send());
                return;
            }

            logger.information("DEBUG: Create project - auth.user=%s, auth.role=%s, has_permission=%d", 
                  auth.login,
                  models::roleToString(auth.role),
                  middleware::AuthMiddleware::hasPermission(auth, "project:write") ? 1 : 0);
            
            // Создаём проект
            models::Project project;
            project.id = storage::nextProjectId++;
            project.name = req.name;
            project.description = req.description;
            project.code = "PROJ-" + std::to_string(project.id);
            project.ownerId = auth.userId;
            
            storage::projects[project.id] = project;
            
            // Ответ
            response.setStatus(Poco::Net::HTTPResponse::HTTP_CREATED);
            dto::CreateProjectResponse::fromProject(project).toJson()->stringify(response.send());
            
            logger.information("201 POST /api/projects - Project %d (%s) created by %s",
                             project.id, project.code.c_str(), auth.login.c_str());
            
        } catch (const std::exception& e) {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
            dto::ErrorResponse::create("invalid_request", e.what(), 400)
                ->stringify(response.send());
        }
    }
    
    // GET /api/projects/search/{name} — Поиск по имени
    else if (path.find("/api/projects/search/") == 0 && method == "GET") {
        std::string name = path.substr(21);  // "/api/projects/search/".length()
        
        Poco::JSON::Array::Ptr results = new Poco::JSON::Array();
        for (const auto& [id, project] : storage::projects) {
            if (project.name.find(name) != std::string::npos) {
                Poco::JSON::Object::Ptr json = new Poco::JSON::Object();
                json->set("id", project.id);
                json->set("name", project.name);
                json->set("description", project.description);
                json->set("code", project.code);
                json->set("ownerId", project.ownerId);
                results->add(json);
            }
        }
        
        response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
        Poco::JSON::Object::Ptr result = new Poco::JSON::Object();
        result->set("projects", results);
        result->stringify(response.send());
        
        logger.information("200 GET /api/projects/search/%s - found %zu projects",
                          name.c_str(), results->size());
    }
    
    // GET /api/projects — Все проекты
    else if (path == "/api/projects" && method == "GET") {
        Poco::JSON::Array::Ptr results = new Poco::JSON::Array();
        for (const auto& [id, project] : storage::projects) {
            if (project.active) {
                Poco::JSON::Object::Ptr json = new Poco::JSON::Object();
                json->set("id", project.id);
                json->set("name", project.name);
                json->set("description", project.description);
                json->set("code", project.code);
                json->set("ownerId", project.ownerId);
                results->add(json);
            }
        }
        
        response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
        Poco::JSON::Object::Ptr result = new Poco::JSON::Object();
        result->set("projects", results);
        result->stringify(response.send());
        
        logger.information("200 GET /api/projects - returned %zu projects", results->size());
    }
    
    else {
        response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
        dto::ErrorResponse::create("not_found", "Project endpoint not found", 404)
            ->stringify(response.send());
    }
}

}