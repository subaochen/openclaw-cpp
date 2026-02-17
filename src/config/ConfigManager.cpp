#include "config/ConfigManager.h"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <locale>
#include <fstream>
#include <sstream>

namespace openclaw {

ConfigManager::ConfigManager()
    : configFile_(""),
      isLoaded_(false) {
}

ConfigManager::~ConfigManager() {
    clear();
}

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::loadConfig(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    clear();
    configFile_ = filename;
    
    if (!parseConfigFile(filename)) {
        isLoaded_ = false;
        configFile_.clear();
        return false;
    }
    
    isLoaded_ = true;
    return true;
}

bool ConfigManager::loadConfigFromDirectory(const std::string& directory) {
    // 简单实现，只加载目录下的第一个配置文件
    std::string filename = directory + "/config.json";
    if (loadConfig(filename)) {
        return true;
    }
    
    filename = directory + "/config.yaml";
    if (loadConfig(filename)) {
        return true;
    }
    
    filename = directory + "/config.ini";
    if (loadConfig(filename)) {
        return true;
    }
    
    filename = directory + "/config.xml";
    if (loadConfig(filename)) {
        return true;
    }
    
    return false;
}

bool ConfigManager::reloadConfig() {
    if (configFile_.empty()) {
        return false;
    }
    
    return loadConfig(configFile_);
}

std::string ConfigManager::getString(const std::string& key, const std::string& defaultValue) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = config_.find(key);
    return it != config_.end() ? it->second : defaultValue;
}

int ConfigManager::getInt(const std::string& key, int defaultValue) const {
    std::string value = getString(key);
    try {
        return std::stoi(value);
    } catch (...) {
        return defaultValue;
    }
}

bool ConfigManager::getBool(const std::string& key, bool defaultValue) const {
    std::string value = getString(key);
    
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    if (value == "true" || value == "yes" || value == "1" || value == "on") {
        return true;
    } else if (value == "false" || value == "no" || value == "0" || value == "off") {
        return false;
    }
    
    return defaultValue;
}

double ConfigManager::getDouble(const std::string& key, double defaultValue) const {
    std::string value = getString(key);
    try {
        return std::stod(value);
    } catch (...) {
        return defaultValue;
    }
}

std::vector<std::string> ConfigManager::getStringList(const std::string& key) const {
    std::vector<std::string> result;
    std::string value = getString(key);
    
    if (value.empty()) {
        return result;
    }
    
    // 简单的CSV解析
    size_t start = 0;
    size_t end = value.find(',');
    
    while (end != std::string::npos) {
        std::string item = value.substr(start, end - start);
        // 移除前导和尾随空格
        item.erase(item.begin(), std::find_if(item.begin(), item.end(),
            [](unsigned char ch) { return !std::isspace(ch); }));
        item.erase(std::find_if(item.rbegin(), item.rend(),
            [](unsigned char ch) { return !std::isspace(ch); }).base(), item.end());
        
        if (!item.empty()) {
            result.push_back(item);
        }
        
        start = end + 1;
        end = value.find(',', start);
    }
    
    // 处理最后一个项
    std::string item = value.substr(start);
    item.erase(item.begin(), std::find_if(item.begin(), item.end(),
        [](unsigned char ch) { return !std::isspace(ch); }));
    item.erase(std::find_if(item.rbegin(), item.rend(),
        [](unsigned char ch) { return !std::isspace(ch); }).base(), item.end());
    
    if (!item.empty()) {
        result.push_back(item);
    }
    
    return result;
}

void ConfigManager::setString(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_[key] = value;
}

void ConfigManager::setInt(const std::string& key, int value) {
    setString(key, std::to_string(value));
}

void ConfigManager::setBool(const std::string& key, bool value) {
    setString(key, value ? "true" : "false");
}

void ConfigManager::setDouble(const std::string& key, double value) {
    setString(key, std::to_string(value));
}

void ConfigManager::setStringList(const std::string& key, const std::vector<std::string>& values) {
    std::string value;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            value += ",";
        }
        value += values[i];
    }
    setString(key, value);
}

bool ConfigManager::hasKey(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_.find(key) != config_.end();
}

std::vector<std::string> ConfigManager::getKeys() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> keys;
    keys.reserve(config_.size());
    
    for (const auto& pair : config_) {
        keys.push_back(pair.first);
    }
    
    std::sort(keys.begin(), keys.end());
    return keys;
}

void ConfigManager::removeKey(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.erase(key);
}

bool ConfigManager::saveConfig(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    // 简单的INI格式保存
    for (const auto& pair : config_) {
        file << pair.first << "=" << pair.second << std::endl;
    }
    
    return true;
}

bool ConfigManager::saveConfig() const {
    if (configFile_.empty()) {
        return saveConfig("config.ini");
    }
    
    return saveConfig(configFile_);
}

void ConfigManager::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.clear();
    configFile_.clear();
    isLoaded_ = false;
}

std::string ConfigManager::getConfigFile() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return configFile_;
}

bool ConfigManager::isLoaded() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return isLoaded_;
}

int ConfigManager::getConfigCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(config_.size());
}

bool ConfigManager::parseConfigFile(const std::string& filename) {
    std::string extension = getFileExtension(filename);
    
    if (extension == "json") {
        return parseJsonFile(filename);
    } else if (extension == "yaml" || extension == "yml") {
        return parseYamlFile(filename);
    } else if (extension == "ini") {
        return parseIniFile(filename);
    } else if (extension == "xml") {
        return parseXmlFile(filename);
    }
    
    // 默认使用INI格式
    return parseIniFile(filename);
}

bool ConfigManager::parseJsonFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    // 简单的JSON解析实现（简化版）
    std::string line;
    while (std::getline(file, line)) {
        // 移除前导和尾随空格
        line.erase(line.begin(), std::find_if(line.begin(), line.end(),
            [](unsigned char ch) { return !std::isspace(ch); }));
        line.erase(std::find_if(line.rbegin(), line.rend(),
            [](unsigned char ch) { return !std::isspace(ch); }).base(), line.end());
        
        if (line.empty() || line[0] == '{' || line[0] == '}' || line[0] == '[') {
            continue;
        }
        
        // 查找键值对
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            continue;
        }
        
        std::string key = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);
        
        // 移除引号
        key.erase(std::remove(key.begin(), key.end(), '"'), key.end());
        key.erase(std::remove(key.begin(), key.end(), '\''), key.end());
        value.erase(std::remove(value.begin(), value.end(), '"'), value.end());
        value.erase(std::remove(value.begin(), value.end(), '\''), value.end());
        
        // 移除键和值的空格
        key.erase(key.begin(), std::find_if(key.begin(), key.end(),
            [](unsigned char ch) { return !std::isspace(ch); }));
        key.erase(std::find_if(key.rbegin(), key.rend(),
            [](unsigned char ch) { return !std::isspace(ch); }).base(), key.end());
        
        value.erase(value.begin(), std::find_if(value.begin(), value.end(),
            [](unsigned char ch) { return !std::isspace(ch); }));
        value.erase(std::find_if(value.rbegin(), value.rend(),
            [](unsigned char ch) { return !std::isspace(ch); }).base(), value.end());
        
        if (!key.empty()) {
            config_[key] = value;
        }
    }
    
    return !config_.empty();
}

bool ConfigManager::parseYamlFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    // 简单的YAML解析实现（简化版）
    std::string line;
    while (std::getline(file, line)) {
        // 移除前导和尾随空格
        line.erase(line.begin(), std::find_if(line.begin(), line.end(),
            [](unsigned char ch) { return !std::isspace(ch); }));
        line.erase(std::find_if(line.rbegin(), line.rend(),
            [](unsigned char ch) { return !std::isspace(ch); }).base(), line.end());
        
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // 查找键值对
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            continue;
        }
        
        std::string key = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);
        
        // 移除键和值的空格
        key.erase(key.begin(), std::find_if(key.begin(), key.end(),
            [](unsigned char ch) { return !std::isspace(ch); }));
        key.erase(std::find_if(key.rbegin(), key.rend(),
            [](unsigned char ch) { return !std::isspace(ch); }).base(), key.end());
        
        value.erase(value.begin(), std::find_if(value.begin(), value.end(),
            [](unsigned char ch) { return !std::isspace(ch); }));
        value.erase(std::find_if(value.rbegin(), value.rend(),
            [](unsigned char ch) { return !std::isspace(ch); }).base(), value.end());
        
        if (!key.empty()) {
            config_[key] = value;
        }
    }
    
    return !config_.empty();
}

bool ConfigManager::parseIniFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // 移除前导和尾随空格
        line.erase(line.begin(), std::find_if(line.begin(), line.end(),
            [](unsigned char ch) { return !std::isspace(ch); }));
        line.erase(std::find_if(line.rbegin(), line.rend(),
            [](unsigned char ch) { return !std::isspace(ch); }).base(), line.end());
        
        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue;
        }
        
        // 查找键值对
        size_t equalsPos = line.find('=');
        if (equalsPos == std::string::npos) {
            continue;
        }
        
        std::string key = line.substr(0, equalsPos);
        std::string value = line.substr(equalsPos + 1);
        
        // 移除键和值的空格
        key.erase(key.begin(), std::find_if(key.begin(), key.end(),
            [](unsigned char ch) { return !std::isspace(ch); }));
        key.erase(std::find_if(key.rbegin(), key.rend(),
            [](unsigned char ch) { return !std::isspace(ch); }).base(), key.end());
        
        value.erase(value.begin(), std::find_if(value.begin(), value.end(),
            [](unsigned char ch) { return !std::isspace(ch); }));
        value.erase(std::find_if(value.rbegin(), value.rend(),
            [](unsigned char ch) { return !std::isspace(ch); }).base(), value.end());
        
        if (!key.empty()) {
            config_[key] = value;
        }
    }
    
    return !config_.empty();
}

bool ConfigManager::parseXmlFile(const std::string& filename) {
    // 简单的XML解析实现（简化版）
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // 移除前导和尾随空格
        line.erase(line.begin(), std::find_if(line.begin(), line.end(),
            [](unsigned char ch) { return !std::isspace(ch); }));
        line.erase(std::find_if(line.rbegin(), line.rend(),
            [](unsigned char ch) { return !std::isspace(ch); }).base(), line.end());
        
        if (line.empty() || line[0] == '<' && (line[1] == '?' || line[1] == '/')) {
            continue;
        }
        
        // 查找标签和内容
        size_t startTag = line.find('<');
        size_t endTag = line.find('>');
        if (startTag == std::string::npos || endTag == std::string::npos) {
            continue;
        }
        
        std::string tagName = line.substr(startTag + 1, endTag - startTag - 1);
        size_t startContent = line.find('>', endTag) + 1;
        size_t endContent = line.find("</" + tagName + ">", startContent);
        
        if (startContent == std::string::npos || endContent == std::string::npos) {
            continue;
        }
        
        std::string content = line.substr(startContent, endContent - startContent);
        
        if (!tagName.empty()) {
            config_[tagName] = content;
        }
    }
    
    return !config_.empty();
}

std::string ConfigManager::getFileExtension(const std::string& filename) const {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos || dotPos == filename.length() - 1) {
        return "";
    }
    
    std::string extension = filename.substr(dotPos + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    return extension;
}

} // namespace openclaw