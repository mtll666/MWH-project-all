#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <map>

// 配置文件解析类
class Config {
public:
    // 构造函数，加载配置文件
    Config(const std::string& filename);

    // 获取字符串配置项
    std::string get(const std::string& key) const;

    // 获取整数配置项
    int get_int(const std::string& key) const;

private:
    std::map<std::string, std::string> settings; // 配置项存储
};

#endif // CONFIG_HPP