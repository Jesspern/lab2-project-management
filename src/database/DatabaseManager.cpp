#include "DatabaseManager.h"

#include <Poco/Data/Session.h>
#include <Poco/Data/SessionFactory.h>
#include <Poco/Data/Statement.h>
#include <Poco/Data/PostgreSQL/Connector.h>
#include <Poco/Environment.h>

using namespace Poco::Data::Keywords;

namespace database {

namespace {
std::string getConnectionString() {
    return Poco::Environment::get(
        "DATABASE_DSN",
        "host=postgres port=5432 dbname=project_management user=postgres password=secret");
}

Poco::Data::Session createSession() {
    return Poco::Data::SessionFactory::instance().create("PostgreSQL", getConnectionString());
}
}

void DatabaseManager::initialize() {
    Poco::Data::PostgreSQL::Connector::registerConnector();
}

void DatabaseManager::shutdown() {
    Poco::Data::PostgreSQL::Connector::unregisterConnector();
}

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager manager;
    return manager;
}

DatabaseManager::DatabaseManager() = default;

std::optional<models::User> DatabaseManager::createUser(const models::User& user) {
    Poco::Data::Session session = createSession();
    models::User created = user;
    std::string role = models::roleToString(user.role);
    session << "INSERT INTO users (login, password, first_name, last_name, email, role) "
               "VALUES ($1, $2, $3, $4, $5, $6) RETURNING id",
        use(created.login), use(created.password), use(created.firstName), use(created.lastName),
        use(created.email), use(role), into(created.id), now;
    return created.id > 0 ? std::optional<models::User>(created) : std::nullopt;
}

std::optional<models::User> DatabaseManager::getUserByLogin(const std::string& login, bool withPassword) {
    Poco::Data::Session session = createSession();
    models::User user;
    std::string role;
    std::string password;
    std::string loginBind = login;
    Poco::Data::Statement select(session);
    select << "SELECT id, login, password, first_name, COALESCE(last_name, ''), "
              "COALESCE(email, ''), role::text FROM users WHERE login = $1",
        use(loginBind), into(user.id), into(user.login), into(password), into(user.firstName),
        into(user.lastName), into(user.email), into(role), range(0, 1);

    if (!select.execute()) {
        return std::nullopt;
    }

    user.password = withPassword ? password : "";
    user.role = models::stringToRole(role);
    return user;
}

std::vector<models::User> DatabaseManager::searchUsers(const std::string& firstName, const std::string& lastName) {
    Poco::Data::Session session = createSession();
    std::vector<models::User> users;

    Poco::Data::Statement select(session);
    int id = 0;
    std::string login;
    std::string first;
    std::string last;
    std::string email;
    std::string role;
    std::string firstBind = firstName;
    std::string lastBind = lastName;

    select << "SELECT id, login, first_name, COALESCE(last_name, ''), COALESCE(email, ''), role::text "
              "FROM users WHERE ($1 = '' OR first_name ILIKE '%' || $1 || '%') "
              "AND ($2 = '' OR last_name ILIKE '%' || $2 || '%') ORDER BY id",
        use(firstBind), use(lastBind), into(id), into(login), into(first), into(last), into(email), into(role),
        range(0, 1);

    while (!select.done()) {
        if (select.execute()) {
            models::User user;
            user.id = id;
            user.login = login;
            user.firstName = first;
            user.lastName = last;
            user.email = email;
            user.role = models::stringToRole(role);
            users.push_back(user);
        }
    }

    return users;
}

bool DatabaseManager::deleteUser(int userId) {
    Poco::Data::Session session = createSession();
    int id = 0;
    Poco::Data::Statement probe(session);
    probe << "SELECT id FROM users WHERE id = $1", use(userId), into(id), range(0, 1);
    if (!probe.execute()) {
        return false;
    }
    session << "DELETE FROM users WHERE id = $1", use(userId), now;
    return true;
}

std::optional<models::Project> DatabaseManager::createProject(const models::Project& project) {
    Poco::Data::Session session = createSession();
    models::Project created = project;
    session << "INSERT INTO projects (name, description, code, owner_id, active) "
               "VALUES ($1, $2, $3, $4, $5) RETURNING id",
        use(created.name), use(created.description), use(created.code), use(created.ownerId), use(created.active),
        into(created.id), now;
    return created.id > 0 ? std::optional<models::Project>(created) : std::nullopt;
}

std::vector<models::Project> DatabaseManager::getProjects(bool onlyActive) {
    Poco::Data::Session session = createSession();
    std::vector<models::Project> projects;
    Poco::Data::Statement select(session);
    int id = 0;
    std::string name;
    std::string description;
    std::string code;
    int ownerId = 0;
    bool active = true;

    if (onlyActive) {
        select << "SELECT id, name, COALESCE(description, ''), code, owner_id, active "
                  "FROM projects WHERE active = TRUE ORDER BY id",
            into(id), into(name), into(description), into(code), into(ownerId), into(active), range(0, 1);
    } else {
        select << "SELECT id, name, COALESCE(description, ''), code, owner_id, active "
                  "FROM projects ORDER BY id",
            into(id), into(name), into(description), into(code), into(ownerId), into(active), range(0, 1);
    }

    while (!select.done()) {
        if (select.execute()) {
            models::Project project;
            project.id = id;
            project.name = name;
            project.description = description;
            project.code = code;
            project.ownerId = ownerId;
            project.active = active;
            projects.push_back(project);
        }
    }
    return projects;
}

std::vector<models::Project> DatabaseManager::searchProjects(const std::string& nameFilter) {
    Poco::Data::Session session = createSession();
    std::vector<models::Project> projects;
    Poco::Data::Statement select(session);
    int id = 0;
    std::string name;
    std::string description;
    std::string code;
    int ownerId = 0;
    bool active = true;
    std::string nameFilterBind = nameFilter;

    select << "SELECT id, name, COALESCE(description, ''), code, owner_id, active "
              "FROM projects WHERE name ILIKE '%' || $1 || '%' ORDER BY id",
        use(nameFilterBind), into(id), into(name), into(description), into(code), into(ownerId), into(active), range(0, 1);

    while (!select.done()) {
        if (select.execute()) {
            models::Project project;
            project.id = id;
            project.name = name;
            project.description = description;
            project.code = code;
            project.ownerId = ownerId;
            project.active = active;
            projects.push_back(project);
        }
    }
    return projects;
}

std::optional<models::Project> DatabaseManager::getProjectById(int projectId) {
    Poco::Data::Session session = createSession();
    models::Project project;
    Poco::Data::Statement select(session);
    select << "SELECT id, name, COALESCE(description, ''), code, owner_id, active "
              "FROM projects WHERE id = $1",
        use(projectId), into(project.id), into(project.name), into(project.description), into(project.code),
        into(project.ownerId), into(project.active), range(0, 1);
    if (!select.execute()) return std::nullopt;
    return project;
}

std::optional<models::Task> DatabaseManager::createTask(const models::Task& task) {
    Poco::Data::Session session = createSession();
    models::Task created = task;
    std::string status = models::taskStatusToString(task.status);
    session << "INSERT INTO tasks (code, title, description, project_id, assignee_id, reporter_id, status) "
               "VALUES ($1, $2, $3, $4, $5, $6, $7) RETURNING id, created_at::text, updated_at::text",
        use(created.code), use(created.title), use(created.description), use(created.projectId),
        use(created.assigneeId), use(created.reporterId), use(status), into(created.id),
        into(created.createdAt), into(created.updatedAt), now;
    return created.id > 0 ? std::optional<models::Task>(created) : std::nullopt;
}

std::vector<models::Task> DatabaseManager::getTasksByProjectId(int projectId) {
    Poco::Data::Session session = createSession();
    std::vector<models::Task> tasks;
    Poco::Data::Statement select(session);
    int id = 0;
    std::string code;
    std::string title;
    std::string description;
    int assigneeId = 0;
    int reporterId = 0;
    std::string status;
    std::string createdAt;
    std::string updatedAt;

    select << "SELECT id, code, title, COALESCE(description, ''), COALESCE(assignee_id, 0), reporter_id, "
              "status::text, created_at::text, updated_at::text "
              "FROM tasks WHERE project_id = $1 ORDER BY created_at DESC",
        use(projectId), into(id), into(code), into(title), into(description), into(assigneeId),
        into(reporterId), into(status), into(createdAt), into(updatedAt), range(0, 1);

    while (!select.done()) {
        if (select.execute()) {
            models::Task task;
            task.id = id;
            task.code = code;
            task.title = title;
            task.description = description;
            task.projectId = projectId;
            task.assigneeId = assigneeId;
            task.reporterId = reporterId;
            task.status = models::stringToTaskStatus(status);
            task.createdAt = createdAt;
            task.updatedAt = updatedAt;
            tasks.push_back(task);
        }
    }
    return tasks;
}

std::optional<models::Task> DatabaseManager::getTaskByCode(const std::string& codeLookup) {
    Poco::Data::Session session = createSession();
    models::Task task;
    std::string status;
    std::string codeBind = codeLookup;
    Poco::Data::Statement select(session);
    select << "SELECT id, code, title, COALESCE(description, ''), project_id, COALESCE(assignee_id, 0), "
              "reporter_id, status::text, created_at::text, updated_at::text "
              "FROM tasks WHERE code = $1",
        use(codeBind), into(task.id), into(task.code), into(task.title), into(task.description),
        into(task.projectId), into(task.assigneeId), into(task.reporterId), into(status),
        into(task.createdAt), into(task.updatedAt), range(0, 1);
    if (!select.execute()) return std::nullopt;
    task.status = models::stringToTaskStatus(status);
    return task;
}

} // namespace database
