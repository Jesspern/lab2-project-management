cmake_minimum_required(VERSION 3.20)
project(jira VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Conan-imported targets often only define Release/Debug; map multi-config IDs to Release libs.
if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)
    set(CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL Release)
endif()

# Poco DataPostgreSQL CMake config expects this imported target to exist first.
find_package(PostgreSQL REQUIRED)

find_package(Poco REQUIRED COMPONENTS Foundation Net Util JSON JWT Data DataPostgreSQL)

add_executable(jira
    src/main.cpp
    
    src/utils/JsonHelper.h

    src/database/DatabaseManager.h
    src/database/DatabaseManager.cpp
    
    src/models/Role.h
    src/models/User.h
    src/models/Project.h
    src/models/Task.h
    
    src/models/dto/CreateUserRequest.h
    src/models/dto/CreateUserResponse.h
    src/models/dto/LoginRequest.h
    src/models/dto/LoginResponse.h
    src/models/dto/ErrorResponse.h
    src/models/dto/CreateProjectRequest.h
    src/models/dto/CreateProjectResponse.h
    src/models/dto/CreateTaskRequest.h
    src/models/dto/CreateTaskResponse.h
    
    src/handlers/UserHandler.cpp
    src/handlers/AuthHandler.cpp
    src/handlers/RouterFactory.cpp
    src/handlers/ProjectHandler.cpp
    src/handlers/TaskHandler.cpp
    
    src/handlers/middleware/AuthMiddleware.h
)

target_link_libraries(jira PRIVATE 
    Poco::Foundation
    Poco::Net
    Poco::Util
    Poco::JSON
    Poco::JWT
    Poco::Data
    Poco::DataPostgreSQL
)

target_include_directories(jira PRIVATE ${CMAKE_SOURCE_DIR}/src)

install(TARGETS jira DESTINATION bin)