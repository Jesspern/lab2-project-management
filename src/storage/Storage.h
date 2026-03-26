#pragma once
#include <map>
#include <string>
#include "../models/User.h"
#include "../models/Project.h"
#include "../models/Task.h"

namespace storage {

extern std::map<int, models::User> users;
extern std::map<std::string, int> loginToId;
extern int nextUserId;

extern std::map<int, models::Project> projects;
extern int nextProjectId;

extern std::map<int, models::Task> tasks;
extern int nextTaskId;

}