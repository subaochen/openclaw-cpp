#ifndef OPENCLAW_CONFIG_MANAGER_H
#define OPENCLAW_CONFIG_MANAGER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <fstream>
#include <optional>

namespace openclaw {

class ConfigManager {
public:
    static ConfigManager& getInstance();

    bool loadConfig(const std::string& filename);
    bool loadConfigFromDirectory(const std::string& directory);
    bool reloadConfig();

    std::string getString(const std::string& key, const std::string& defaultValue = "") const;
    int getInt(const std::string& key, int defaultValue = 0) const;
    bool getBool(const std::string& key, bool defaultValue = false) const;
    double getDouble(const std::string& key, double defaultValue = 0.0) const;
    std::vector<std::string> getStringList(const std::string& key) const;

    void setString(const std::string& key, const std::string& value);
    void setInt(const std::string& key, int value);
    void setBool(const std::string& key, bool value);
    void setDouble(const std::string& key, double value);
    void setStringList(const std::string& key, const std::vector<std::string>& values);

    bool hasKey(const std::string& key) const;
    std::vector<std::string> getKeys() const;
    void removeKey(const std::string& key);

    bool saveConfig(const std::string& filename) const;
    bool saveConfig() const;

    void clear();

    std::string getConfigFile() const;
    bool isLoaded() const;
    int getConfigCount() const;

private:
    ConfigManager();
    ~ConfigManager();

    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    ConfigManager(ConfigManager&&) = delete;
    ConfigManager& operator=(ConfigManager&&) = delete;

    bool parseConfigFile(const std::string& filename);
    bool parseJsonFile(const std::string& filename);
    bool parseYamlFile(const std::string& filename);
    bool parseIniFile(const std::string& filename);
    bool parseXmlFile(const std::string& filename);

    std::string getFileExtension(const std::string& filename) const;

    std::unordered_map<std::string, std::string> config_;
    std::string configFile_;
    mutable std::mutex mutex_;
    bool isLoaded_;
};

} // namespace openclaw

#endif // OPENCLAW_CONFIG_MANAGER_H