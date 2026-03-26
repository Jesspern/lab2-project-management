#pragma once
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Array.h>
#include <Poco/Exception.h>
#include <Poco/URI.h>
#include <string>
#include <sstream>
#include <vector>

namespace utils {

template<typename T>
T getOrDefaultImpl(const Poco::JSON::Object& json, 
                   const std::string& key, 
                   const T& defaultValue) {
    if (json.has(key) && !json.isNull(key)) {
        return json.getValue<T>(key);
    }
    return defaultValue;
}

template<typename T>
T getOrDefault(const Poco::JSON::Object& json, 
               const std::string& key, 
               const T& defaultValue) {
    return getOrDefaultImpl<T>(json, key, defaultValue);
}

inline std::string getString(const Poco::JSON::Object& json,
                             const std::string& key,
                             const std::string& defaultValue = "") {
    return getOrDefaultImpl<std::string>(json, key, defaultValue);
}

inline int getInt(const Poco::JSON::Object& json,
                  const std::string& key,
                  int defaultValue = 0) {
    return getOrDefaultImpl<int>(json, key, defaultValue);
}

inline bool getBool(const Poco::JSON::Object& json,
                    const std::string& key,
                    bool defaultValue = false) {
    return getOrDefaultImpl<bool>(json, key, defaultValue);
}

template<typename T>
T getOrDefault(const Poco::JSON::Object::Ptr& jsonPtr, 
               const std::string& key, 
               const T& defaultValue) {
    if (!jsonPtr) return defaultValue;
    return getOrDefaultImpl<T>(*jsonPtr, key, defaultValue);
}

inline std::string getString(const Poco::JSON::Object::Ptr& jsonPtr,
                             const std::string& key,
                             const std::string& defaultValue = "") {
    if (!jsonPtr) return defaultValue;
    return getString(*jsonPtr, key, defaultValue);
}

inline int getInt(const Poco::JSON::Object::Ptr& jsonPtr,
                  const std::string& key,
                  int defaultValue = 0) {
    if (!jsonPtr) return defaultValue;
    return getInt(*jsonPtr, key, defaultValue);
}

inline bool getBool(const Poco::JSON::Object::Ptr& jsonPtr,
                    const std::string& key,
                    bool defaultValue = false) {
    if (!jsonPtr) return defaultValue;
    return getBool(*jsonPtr, key, defaultValue);
}

template<typename T>
T getRequired(const Poco::JSON::Object& json, const std::string& key) {
    if (!json.has(key) || json.isNull(key)) {
        throw Poco::NotFoundException("Required field '" + key + "' not found");
    }
    return json.getValue<T>(key);
}

template<typename T>
T getRequired(const Poco::JSON::Object::Ptr& jsonPtr, const std::string& key) {
    if (!jsonPtr) {
        throw Poco::NotFoundException("JSON object is null");
    }
    return getRequired<T>(*jsonPtr, key);
}

inline bool hasValue(const Poco::JSON::Object& json, const std::string& key) {
    return json.has(key) && !json.isNull(key);
}

inline bool hasValue(const Poco::JSON::Object::Ptr& jsonPtr, const std::string& key) {
    return jsonPtr && jsonPtr->has(key) && !jsonPtr->isNull(key);
}

inline std::string stringify(const Poco::JSON::Object& json) {
    std::ostringstream ostr;
    Poco::JSON::Stringifier::stringify(json, ostr);
    return ostr.str();
}

inline std::string stringify(const Poco::JSON::Object::Ptr& jsonPtr) {
    if (!jsonPtr) return "{}";
    return stringify(*jsonPtr);
}

inline std::string getQueryParam(
    const Poco::URI::QueryParameters& params,
    const std::string& key,
    const std::string& defaultValue = "") {
    
    for (const auto& param : params) {
        if (param.first == key) {
            return param.second;
        }
    }
    return defaultValue;
}

inline std::string getQueryParam(
    const Poco::URI& uri,
    const std::string& key,
    const std::string& defaultValue = "") {
    
    return getQueryParam(uri.getQueryParameters(), key, defaultValue);
}

inline bool hasQueryParam(
    const Poco::URI::QueryParameters& params,
    const std::string& key) {
    
    for (const auto& param : params) {
        if (param.first == key) {
            return true;
        }
    }
    return false;
}

inline bool hasQueryParam(const Poco::URI& uri, const std::string& key) {
    return hasQueryParam(uri.getQueryParameters(), key);
}

}