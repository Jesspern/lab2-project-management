#include "Storage.h"

namespace storage {

std::map<int, models::User> users;
std::map<std::string, int> loginToId;
int nextUserId = 1;

std::map<int, models::Project> projects;
int nextProjectId = 1;

std::map<int, models::Task> tasks;
int nextTaskId = 1;

}