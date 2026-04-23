#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../models/Project.h"
#include "../models/Task.h"
#include "../models/User.h"

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>

namespace database {

class MongoDBManager {
public:
    static void initialize();
    static void shutdown();
    static MongoDBManager& instance();

    std::optional<models::User> createUser(const models::User& user);
    std::optional<models::User> getUserByLogin(const std::string& login, bool withPassword = false);
    std::vector<models::User> searchUsers(const std::string& firstName, const std::string& lastName);
    bool deleteUser(int userId);

    std::optional<models::Project> createProject(const models::Project& project);
    std::vector<models::Project> getProjects(bool onlyActive = true);
    std::vector<models::Project> searchProjects(const std::string& name);
    std::optional<models::Project> getProjectById(int projectId);

    std::optional<models::Task> createTask(const models::Task& task);
    std::vector<models::Task> getTasksByProjectId(int projectId);
    std::optional<models::Task> getTaskByCode(const std::string& code);

private:
    MongoDBManager();
    int nextId(const std::string& counterName, const std::string& collectionName);
    void ensureIndexes();

private:
    mongocxx::client client_;
    mongocxx::database db_;
};

} // namespace database
