#include "TaskHandler.h"
#include "../models/Task.h"
#include "../models/Project.h"
#include "../models/dto/CreateTaskRequest.h"
#include "../models/dto/CreateTaskResponse.h"
#include "../models/dto/ErrorResponse.h"
#include "../utils/JsonHelper.h"
#include "../handlers/middleware/AuthMiddleware.h"
#include "../database/MongoDBManager.h"
#include <Poco/Logger.h>
#include <Poco/Timestamp.h>
#include <Poco/DateTime.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/DateTimeFormat.h>
#include <Poco/JSON/Parser.h>
#include <Poco/URI.h>
#include <sstream>

namespace handlers {

void TaskHandler::handleRequest(Poco::Net::HTTPServerRequest& request,
                                 Poco::Net::HTTPServerResponse& response) {
    response.setContentType("application/json");
    response.set("Access-Control-Allow-Origin", "*");
    
    const std::string& method = request.getMethod();
    Poco::URI uri(request.getURI());
    std::string path = uri.getPath();
    
    auto& logger = Poco::Logger::get("ProjectAPI");
    logger.information("%s %s", method, path);
    
    // POST /api/projects/{projectId}/tasks — Создание задачи
    // POST /api/projects/{projectId}/tasks — Создание задачи
    if (path.find("/api/projects/") == 0 && path.find("/tasks") != std::string::npos && 
        method == "POST") {
        
        // Проверка авторизации
        auto auth = middleware::AuthMiddleware::verifyToken(request);
        if (!auth.authenticated) {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED);
            dto::ErrorResponse::create("unauthorized", "Login required", 401)
                ->stringify(response.send());
            logger.warning("401 POST /api/projects/{id}/tasks - not authenticated");
            return;
        }
        
        // Проверка прав
        if (!middleware::AuthMiddleware::hasPermission(auth, "task:write")) {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_FORBIDDEN);
            dto::ErrorResponse::create("forbidden", "Insufficient permissions", 403)
                ->stringify(response.send());
            logger.warning("403 POST /api/projects/{id}/tasks - User %s denied", auth.login);
            return;
        }
        
        try {
            // ===== Исправленный парсинг projectId =====
            size_t prefix_len = 14;  // "/api/projects/"
            size_t suffix_pos = path.find("/tasks", prefix_len);
            
            if (suffix_pos == std::string::npos || suffix_pos <= prefix_len) {
                logger.warning("400 POST /api/projects/{id}/tasks - Invalid path: %s", path.c_str());
                response.setStatus(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
                dto::ErrorResponse::create("invalid_path", "Invalid task endpoint", 400)
                    ->stringify(response.send());
                return;
            }
            
            std::string idStr = path.substr(prefix_len, suffix_pos - prefix_len);
            int projectId = std::stoi(idStr);
            // ===== Конец парсинга =====
            
            // Проверяем существование проекта
            if (!database::MongoDBManager::instance().getProjectById(projectId).has_value()) {
                response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
                dto::ErrorResponse::create("not_found", "Project not found", 404)
                    ->stringify(response.send());
                logger.warning("404 POST /api/projects/%d/tasks - project not found", projectId);
                return;
            }
            
            // Парсим запрос через DTO
            Poco::JSON::Parser parser;
            auto json = parser.parse(request.stream()).extract<Poco::JSON::Object::Ptr>();
            dto::CreateTaskRequest req = dto::CreateTaskRequest::fromJson(json);
            
            if (!req.isValid()) {
                response.setStatus(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
                dto::ErrorResponse::create("validation_error", "title is required", 400)
                    ->stringify(response.send());
                logger.warning("400 POST /api/projects/%d/tasks - validation failed", projectId);
                return;
            }
            
            // Создаём задачу
            models::Task task;
            task.code = "PROJ-" + std::to_string(projectId) + "-TASK-" + std::to_string(Poco::Timestamp().epochMicroseconds());
            task.title = req.title;
            task.description = req.description;
            task.projectId = projectId;
            task.assigneeId = req.assigneeId;
            task.reporterId = auth.userId;
            task.status = models::TaskStatus::NEW;
            
            auto created = database::MongoDBManager::instance().createTask(task);
            if (!created.has_value()) {
                response.setStatus(Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
                dto::ErrorResponse::create("db_error", "Failed to create task", 500)
                    ->stringify(response.send());
                return;
            }
            task = created.value();
            
            // Ответ
            response.setStatus(Poco::Net::HTTPResponse::HTTP_CREATED);
            dto::CreateTaskResponse::fromTask(task).toJson()->stringify(response.send());
            
            logger.information("201 POST /api/projects/%d/tasks - Task %s created by %s",
                            projectId, task.code.c_str(), auth.login.c_str());
            
        } catch (const std::invalid_argument& e) {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
            dto::ErrorResponse::create("invalid_id", "Invalid project ID format", 400)
                ->stringify(response.send());
            logger.warning("400 POST /api/projects/{id}/tasks - invalid ID: %s", e.what());
        } catch (const std::out_of_range& e) {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
            dto::ErrorResponse::create("invalid_id", "Project ID out of range", 400)
                ->stringify(response.send());
            logger.warning("400 POST /api/projects/{id}/tasks - ID out of range");
        } catch (const Poco::Exception& e) {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
            dto::ErrorResponse::create("invalid_json", "Invalid JSON format", 400)
                ->stringify(response.send());
            logger.warning("400 POST /api/projects/{id}/tasks - %s", e.displayText());
        } catch (const std::exception& e) {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
            dto::ErrorResponse::create("internal_error", "Internal error", 500)
                ->stringify(response.send());
            logger.error("500 POST /api/projects/{id}/tasks - %s", e.what());
        }
    }
    
    // GET /api/projects/{projectId}/tasks — Все задачи проекта
    else if (path.find("/api/projects/") == 0 && path.find("/tasks") != std::string::npos && 
         method == "GET") {
    
        // Проверка авторизации
        auto auth = middleware::AuthMiddleware::verifyToken(request);
        if (!auth.authenticated) {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED);
            dto::ErrorResponse::create("unauthorized", "Login required", 401)
                ->stringify(response.send());
            logger.warning("401 GET /api/projects/{id}/tasks - not authenticated");
            return;
        }
        
        try {
            // ===== Исправленный парсинг projectId =====
            size_t prefix_len = 14;  // "/api/projects/"
            size_t suffix_pos = path.find("/tasks", prefix_len);
            
            if (suffix_pos == std::string::npos || suffix_pos <= prefix_len) {
                logger.warning("400 GET /api/projects/{id}/tasks - Invalid path: %s", path.c_str());
                response.setStatus(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
                dto::ErrorResponse::create("invalid_path", "Invalid task endpoint", 400)
                    ->stringify(response.send());
                return;
            }
            
            std::string idStr = path.substr(prefix_len, suffix_pos - prefix_len);
            int projectId = std::stoi(idStr);
            // ===== Конец парсинга =====
            
            Poco::JSON::Array::Ptr results = new Poco::JSON::Array();
            auto tasks = database::MongoDBManager::instance().getTasksByProjectId(projectId);
            for (const auto& task : tasks) {
                Poco::JSON::Object::Ptr json = new Poco::JSON::Object();
                json->set("id", task.id);
                json->set("code", task.code);
                json->set("title", task.title);
                json->set("description", task.description);
                json->set("status", models::taskStatusToString(task.status));
                json->set("assigneeId", task.assigneeId);
                json->set("reporterId", task.reporterId);
                json->set("createdAt", task.createdAt);
                json->set("updatedAt", task.updatedAt);
                results->add(json);
            }
            
            response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
            Poco::JSON::Object::Ptr result = new Poco::JSON::Object();
            result->set("tasks", results);
            result->stringify(response.send());
            
            // Исправлено: .c_str() для std::string
            logger.information("200 GET /api/projects/%d/tasks - found %zu tasks",
                            projectId, results->size());
            
        } catch (const std::invalid_argument& e) {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
            dto::ErrorResponse::create("invalid_id", "Invalid project ID format", 400)
                ->stringify(response.send());
            logger.warning("400 GET /api/projects/{id}/tasks - invalid ID");
        } catch (const std::exception& e) {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
            dto::ErrorResponse::create("internal_error", "Internal error", 500)
                ->stringify(response.send());
            logger.error("500 GET /api/projects/{id}/tasks - %s", e.what());
        }
    }
    
    // GET /api/tasks/{taskCode} — Задача по коду
    else if (path.find("/api/tasks/") == 0 && method == "GET") {
        std::string taskCode = path.substr(11);  // "/api/tasks/".length()
        
        auto taskOpt = database::MongoDBManager::instance().getTaskByCode(taskCode);
        if (taskOpt.has_value()) {
            const auto& task = taskOpt.value();
            Poco::JSON::Object::Ptr json = new Poco::JSON::Object();
            json->set("id", task.id);
            json->set("code", task.code);
            json->set("title", task.title);
            json->set("description", task.description);
            json->set("status", models::taskStatusToString(task.status));
            json->set("projectId", task.projectId);
            json->set("assigneeId", task.assigneeId);
            json->set("reporterId", task.reporterId);
            json->set("createdAt", task.createdAt);
            json->set("updatedAt", task.updatedAt);
            
            response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
            json->stringify(response.send());
            
            logger.information("200 GET /api/tasks/%s", taskCode.c_str());
            return;
        }
        
        response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
        dto::ErrorResponse::create("not_found", "Task not found", 404)
            ->stringify(response.send());
        logger.warning("404 GET /api/tasks/%s - not found", taskCode.c_str());
    }
    
    // Неизвестный endpoint
    else {
        response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
        dto::ErrorResponse::create("not_found", "Task endpoint not found", 404)
            ->stringify(response.send());
    }
}

}