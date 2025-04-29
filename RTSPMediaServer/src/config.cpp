#include "config.hpp"
#include <fstream>
#include <sstream>
#include "logger.hpp"

Config::Config(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        Logger::error("无法打开配置文件: " + filename);
        return;
    }

    // 解析配置文件
    std::string line, section;
    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        if (line[0] == '[' && line[line.size() - 1] == ']') {
            section = line.substr(1, line.size() - 2);
            continue;
        }
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        std::string key = section + "." + line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        settings[key] = value;
    }
    file.close();
}

std::string Config::get(const std::string& key) const {
    auto it = settings.find(key);
    return it != settings.end() ? it->second : "";
}

int Config::get_int(const std::string& key) const {
    std::string value = get(key);
    try {
        return value.empty() ? 0 : std::stoi(value);
    } catch (...) {
        Logger::error("无效的整数配置项: " + key);
        return 0;
    }
}