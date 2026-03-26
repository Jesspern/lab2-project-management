#pragma once
#include <string>
#include <vector>

namespace models {

struct Project {
    int id;
    std::string name;
    std::string description;
    std::string code;
    int ownerId;
    std::vector<int> memberIds;
    bool active;
    
    Project() : id(0), ownerId(0), active(true) {}
    
    Project(int id, const std::string& name, const std::string& description,
            const std::string& code, int ownerId)
        : id(id), name(name), description(description), 
          code(code), ownerId(ownerId), active(true) {}
};

}