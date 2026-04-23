#include "MongoDBManager.h"

#include <chrono>
#include <iomanip>
#include <sstream>

#include <Poco/Environment.h>

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/options/find.hpp>
#include <mongocxx/options/find_one_and_update.hpp>
#include <mongocxx/options/index.hpp>
#include <mongocxx/uri.hpp>

using bsoncxx::builder::basic::array;
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::make_document;

namespace database {
namespace {
std::unique_ptr<mongocxx::instance> g_instance;

std::string nowIso() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t raw = std::chrono::system_clock::to_time_t(now);
    std::tm utc {};
#if defined(_WIN32)
    gmtime_s(&utc, &raw);
#else
    gmtime_r(&raw, &utc);
#endif
    std::ostringstream out;
    out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

bsoncxx::types::b_date nowDate() {
    return bsoncxx::types::b_date(std::chrono::system_clock::now());
}

std::string dateToIso(const bsoncxx::types::b_date& value) {
    const auto tp = std::chrono::system_clock::time_point(value.value);
    const auto time = std::chrono::system_clock::to_time_t(tp);
    std::tm utc {};
#if defined(_WIN32)
    gmtime_s(&utc, &time);
#else
    gmtime_r(&time, &utc);
#endif
    std::ostringstream out;
    out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

std::string getStringOr(const bsoncxx::document::view& doc, const std::string& key, const std::string& fallback = "") {
    const auto el = doc[key];
    return (el && el.type() == bsoncxx::type::k_string) ? el.get_string().value.to_string() : fallback;
}

int getIntOr(const bsoncxx::document::view& doc, const std::string& key, int fallback = 0) {
    const auto el = doc[key];
    if (!el) return fallback;
    if (el.type() == bsoncxx::type::k_int32) return el.get_int32().value;
    if (el.type() == bsoncxx::type::k_int64) return static_cast<int>(el.get_int64().value);
    return fallback;
}
} // namespace

void MongoDBManager::initialize() {
    if (!g_instance) {
        g_instance = std::make_unique<mongocxx::instance>();
    }
}

void MongoDBManager::shutdown() {
    g_instance.reset();
}

MongoDBManager& MongoDBManager::instance() {
    static MongoDBManager manager;
    return manager;
}

MongoDBManager::MongoDBManager()
    : client_(mongocxx::uri(Poco::Environment::get("MONGODB_URI", "mongodb://mongodb:27017"))),
      db_(client_[Poco::Environment::get("MONGODB_DATABASE", "project_management")]) {
    ensureIndexes();
}

void MongoDBManager::ensureIndexes() {
    mongocxx::options::index unique;
    unique.unique(true);

    db_["users"].create_index(make_document(kvp("id", 1)), unique);
    db_["users"].create_index(make_document(kvp("login", 1)), unique);
    db_["projects"].create_index(make_document(kvp("id", 1)), unique);
    db_["projects"].create_index(make_document(kvp("code", 1)), unique);
    db_["tasks"].create_index(make_document(kvp("id", 1)), unique);
    db_["tasks"].create_index(make_document(kvp("code", 1)), unique);
    db_["tasks"].create_index(make_document(kvp("projectId", 1)));
}

int MongoDBManager::nextId(const std::string& counterName, const std::string& collectionName) {
    auto counters = db_["counters"];
    auto current = counters.find_one(make_document(kvp("_id", counterName)));
    if (!current) {
        int maxId = 0;
        mongocxx::options::find opts;
        opts.sort(make_document(kvp("id", -1)));
        opts.limit(1);
        auto cursor = db_[collectionName].find({}, opts);
        for (const auto& doc : cursor) {
            maxId = getIntOr(doc, "id", 0);
        }
        counters.insert_one(make_document(kvp("_id", counterName), kvp("seq", maxId)));
    }

    mongocxx::options::find_one_and_update opts;
    opts.return_document(mongocxx::options::return_document::k_after);
    opts.upsert(true);

    auto updated = counters.find_one_and_update(
        make_document(kvp("_id", counterName)),
        make_document(kvp("$inc", make_document(kvp("seq", 1)))),
        opts);
    if (!updated) return 1;
    return getIntOr(updated->view(), "seq", 1);
}

std::optional<models::User> MongoDBManager::createUser(const models::User& user) {
    models::User created = user;
    created.id = nextId("users", "users");
    db_["users"].insert_one(
        make_document(
            kvp("id", created.id),
            kvp("login", created.login),
            kvp("password", created.password),
            kvp("firstName", created.firstName),
            kvp("lastName", created.lastName),
            kvp("email", created.email),
            kvp("role", models::roleToString(created.role)),
            kvp("createdAt", nowDate())));
    return created;
}

std::optional<models::User> MongoDBManager::getUserByLogin(const std::string& login, bool withPassword) {
    auto doc = db_["users"].find_one(make_document(kvp("login", login)));
    if (!doc) return std::nullopt;
    const auto view = doc->view();
    models::User user;
    user.id = getIntOr(view, "id");
    user.login = getStringOr(view, "login");
    user.password = withPassword ? getStringOr(view, "password") : "";
    user.firstName = getStringOr(view, "firstName");
    user.lastName = getStringOr(view, "lastName");
    user.email = getStringOr(view, "email");
    user.role = models::stringToRole(getStringOr(view, "role", "GUEST"));
    return user;
}

std::vector<models::User> MongoDBManager::searchUsers(const std::string& firstName, const std::string& lastName) {
    bsoncxx::builder::basic::document filter;
    array conditions;
    if (!firstName.empty()) {
        conditions.append(make_document(kvp("firstName", bsoncxx::types::b_regex{firstName, "i"})));
    }
    if (!lastName.empty()) {
        conditions.append(make_document(kvp("lastName", bsoncxx::types::b_regex{lastName, "i"})));
    }
    if (!firstName.empty() || !lastName.empty()) {
        filter.append(kvp("$and", conditions));
    }

    std::vector<models::User> users;
    auto cursor = db_["users"].find(filter.view());
    for (const auto& doc : cursor) {
        models::User user;
        user.id = getIntOr(doc, "id");
        user.login = getStringOr(doc, "login");
        user.firstName = getStringOr(doc, "firstName");
        user.lastName = getStringOr(doc, "lastName");
        user.email = getStringOr(doc, "email");
        user.role = models::stringToRole(getStringOr(doc, "role", "GUEST"));
        users.push_back(user);
    }
    return users;
}

bool MongoDBManager::deleteUser(int userId) {
    auto result = db_["users"].delete_one(make_document(kvp("id", userId)));
    return result && result->deleted_count() > 0;
}

std::optional<models::Project> MongoDBManager::createProject(const models::Project& project) {
    models::Project created = project;
    created.id = nextId("projects", "projects");
    db_["projects"].insert_one(
        make_document(
            kvp("id", created.id),
            kvp("name", created.name),
            kvp("description", created.description),
            kvp("code", created.code),
            kvp("ownerId", created.ownerId),
            kvp("memberIds", make_array()),
            kvp("active", created.active),
            kvp("createdAt", nowDate()),
            kvp("updatedAt", nowDate())));
    return created;
}

std::vector<models::Project> MongoDBManager::getProjects(bool onlyActive) {
    bsoncxx::builder::basic::document filter;
    if (onlyActive) filter.append(kvp("active", true));

    std::vector<models::Project> projects;
    auto cursor = db_["projects"].find(filter.view());
    for (const auto& doc : cursor) {
        models::Project project;
        project.id = getIntOr(doc, "id");
        project.name = getStringOr(doc, "name");
        project.description = getStringOr(doc, "description");
        project.code = getStringOr(doc, "code");
        project.ownerId = getIntOr(doc, "ownerId");
        project.active = doc["active"] ? doc["active"].get_bool().value : true;
        projects.push_back(project);
    }
    return projects;
}

std::vector<models::Project> MongoDBManager::searchProjects(const std::string& name) {
    std::vector<models::Project> projects;
    auto cursor = db_["projects"].find(make_document(kvp("name", bsoncxx::types::b_regex{name, "i"})));
    for (const auto& doc : cursor) {
        models::Project project;
        project.id = getIntOr(doc, "id");
        project.name = getStringOr(doc, "name");
        project.description = getStringOr(doc, "description");
        project.code = getStringOr(doc, "code");
        project.ownerId = getIntOr(doc, "ownerId");
        project.active = doc["active"] ? doc["active"].get_bool().value : true;
        projects.push_back(project);
    }
    return projects;
}

std::optional<models::Project> MongoDBManager::getProjectById(int projectId) {
    auto doc = db_["projects"].find_one(make_document(kvp("id", projectId)));
    if (!doc) return std::nullopt;
    models::Project project;
    const auto view = doc->view();
    project.id = getIntOr(view, "id");
    project.name = getStringOr(view, "name");
    project.description = getStringOr(view, "description");
    project.code = getStringOr(view, "code");
    project.ownerId = getIntOr(view, "ownerId");
    project.active = view["active"] ? view["active"].get_bool().value : true;
    return project;
}

std::optional<models::Task> MongoDBManager::createTask(const models::Task& task) {
    models::Task created = task;
    created.id = nextId("tasks", "tasks");
    created.createdAt = nowIso();
    created.updatedAt = created.createdAt;

    db_["tasks"].insert_one(
        make_document(
            kvp("id", created.id),
            kvp("code", created.code),
            kvp("title", created.title),
            kvp("description", created.description),
            kvp("projectId", created.projectId),
            kvp("assigneeId", created.assigneeId),
            kvp("reporterId", created.reporterId),
            kvp("status", models::taskStatusToString(created.status)),
            kvp("priority", "MEDIUM"),
            kvp("tags", make_array()),
            kvp("comments", make_array()),
            kvp("history", make_array()),
            kvp("createdAt", nowDate()),
            kvp("updatedAt", nowDate())));
    return created;
}

std::vector<models::Task> MongoDBManager::getTasksByProjectId(int projectId) {
    std::vector<models::Task> tasks;
    mongocxx::options::find opts;
    opts.sort(make_document(kvp("createdAt", -1)));

    auto cursor = db_["tasks"].find(make_document(kvp("projectId", projectId)), opts);
    for (const auto& doc : cursor) {
        models::Task task;
        task.id = getIntOr(doc, "id");
        task.code = getStringOr(doc, "code");
        task.title = getStringOr(doc, "title");
        task.description = getStringOr(doc, "description");
        task.projectId = getIntOr(doc, "projectId");
        task.assigneeId = getIntOr(doc, "assigneeId");
        task.reporterId = getIntOr(doc, "reporterId");
        task.status = models::stringToTaskStatus(getStringOr(doc, "status", "NEW"));
        task.createdAt = doc["createdAt"] ? dateToIso(doc["createdAt"].get_date()) : nowIso();
        task.updatedAt = doc["updatedAt"] ? dateToIso(doc["updatedAt"].get_date()) : task.createdAt;
        tasks.push_back(task);
    }
    return tasks;
}

std::optional<models::Task> MongoDBManager::getTaskByCode(const std::string& code) {
    auto doc = db_["tasks"].find_one(make_document(kvp("code", code)));
    if (!doc) return std::nullopt;
    models::Task task;
    const auto view = doc->view();
    task.id = getIntOr(view, "id");
    task.code = getStringOr(view, "code");
    task.title = getStringOr(view, "title");
    task.description = getStringOr(view, "description");
    task.projectId = getIntOr(view, "projectId");
    task.assigneeId = getIntOr(view, "assigneeId");
    task.reporterId = getIntOr(view, "reporterId");
    task.status = models::stringToTaskStatus(getStringOr(view, "status", "NEW"));
    task.createdAt = view["createdAt"] ? dateToIso(view["createdAt"].get_date()) : nowIso();
    task.updatedAt = view["updatedAt"] ? dateToIso(view["updatedAt"].get_date()) : task.createdAt;
    return task;
}

} // namespace database
