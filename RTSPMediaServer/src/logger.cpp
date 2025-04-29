#include "logger.hpp"
#include <sstream>

void Logger::info(const std::string& msg) {
    std::cout << get_timestamp() << " [INFO] " << msg << std::endl;
}

void Logger::error(const std::string& msg) {
    std::cerr << get_timestamp() << " [ERROR] " << msg << std::endl;
}

std::string Logger::get_timestamp() {
    auto now = std::time(nullptr);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}