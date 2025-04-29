#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <iostream>
#include <ctime>
#include <iomanip>

// 日志类，记录服务端运行信息
class Logger {
public:
    // 输出INFO级别日志
    static void info(const std::string& msg);

    // 输出ERROR级别日志
    static void error(const std::string& msg);

private:
    // 获取当前时间戳
    static std::string get_timestamp();
};

#endif // LOGGER_HPP