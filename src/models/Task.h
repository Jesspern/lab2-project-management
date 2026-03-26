#pragma once
#include <string>
#include <Poco/Timestamp.h>
#include <Poco/DateTime.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/DateTimeFormat.h>
#include "Role.h"

namespace models {

enum class TaskStatus {
    NEW,
    IN_PROGRESS,
    REVIEW,
    DONE,
    CANCELLED
};

inline std::string taskStatusToString(TaskStatus status) {
    switch (status) {
        case TaskStatus::NEW: return "NEW";
        case TaskStatus::IN_PROGRESS: return "IN_PROGRESS";
        case TaskStatus::REVIEW: return "REVIEW";
        case TaskStatus::DONE: return "DONE";
        case TaskStatus::CANCELLED: return "CANCELLED";
        default: return "UNKNOWN";
    }
}

inline TaskStatus stringToTaskStatus(const std::string& str) {
    if (str == "NEW") return TaskStatus::NEW;
    if (str == "IN_PROGRESS") return TaskStatus::IN_PROGRESS;
    if (str == "REVIEW") return TaskStatus::REVIEW;
    if (str == "DONE") return TaskStatus::DONE;
    if (str == "CANCELLED") return TaskStatus::CANCELLED;
    return TaskStatus::NEW;
}

inline std::string formatTimestamp(const Poco::Timestamp& ts) {
    Poco::DateTime dt(ts);
    return Poco::DateTimeFormatter::format(dt, Poco::DateTimeFormat::ISO8601_FORMAT);
}

struct Task {
    int id;
    std::string code;
    std::string title;
    std::string description;
    int projectId;
    int assigneeId;
    int reporterId;
    TaskStatus status;
    std::string createdAt;
    std::string updatedAt;
    
    Task() : id(0), projectId(0), assigneeId(0), reporterId(0), 
             status(TaskStatus::NEW) {
        // Устанавливаем время создания по умолчанию
        Poco::Timestamp now;
        createdAt = formatTimestamp(now);
        updatedAt = formatTimestamp(now);
    }
    
    // Обновить время изменения
    void touch() {
        Poco::Timestamp now;
        updatedAt = formatTimestamp(now);
    }
};

}