#pragma once
#include <string>
#include <unordered_set>

namespace models {

enum class Role {
    GUEST,        // Неавторизованный пользователь
    WORKER,       // Исполнитель: видит свои задачи, создаёт задачи
    TRACKER,      // Руководитель: управляет задачами, проектами, релизами
    ADMINISTRATOR // Админ: полный доступ, управление пользователями
};

// Конвертация строки → роль (из JWT токена)
inline Role stringToRole(const std::string& str) {
    if (str == "ADMINISTRATOR" || str == "ADMIN") return Role::ADMINISTRATOR;
    if (str == "TRACKER") return Role::TRACKER;
    if (str == "WORKER") return Role::WORKER;
    return Role::GUEST;
}

// Конвертация роль → строка (для ответа API)
inline std::string roleToString(Role role) {
    switch (role) {
        case Role::ADMINISTRATOR: return "ADMINISTRATOR";
        case Role::TRACKER: return "TRACKER";
        case Role::WORKER: return "WORKER";
        default: return "GUEST";
    }
}

// Проверка прав доступа
inline bool hasPermission(Role role, const std::string& permission) {
    // Матрица прав доступа
    static const std::unordered_set<std::string> adminPermissions = {
        "user:read", "user:write", "user:delete",
        "project:read", "project:write", "project:delete",
        "task:read", "task:write", "task:delete",
        "system:admin"
    };
    
    static const std::unordered_set<std::string> trackerPermissions = {
        "user:read",
        "project:read", "project:write",
        "task:read", "task:write", "task:delete",
        "release:manage"
    };
    
    static const std::unordered_set<std::string> workerPermissions = {
        "user:read",
        "project:read",
        "task:read", "task:write" 
    };
    
    if (role == Role::ADMINISTRATOR) {
        return adminPermissions.count(permission) > 0;
    }
    if (role == Role::TRACKER) {
        return trackerPermissions.count(permission) > 0;
    }
    if (role == Role::WORKER) {
        return workerPermissions.count(permission) > 0;
    }
    return false;  // GUEST не имеет прав
}

}