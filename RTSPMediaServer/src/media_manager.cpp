#include "media_manager.hpp"
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

MediaManager::MediaManager(const std::string& media_dir, const std::string& data_dir)
    : media_dir_(media_dir), data_dir_(data_dir) {
    // 初始化时加载播放列表和历史记录
    load_playlist();
    load_history();
}

bool MediaManager::add_media(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!is_valid_media(filename)) {
        Logger::error("无效的媒体文件: " + filename);
        return false;
    }
    std::string full_path = media_dir_ + filename;
    if (std::find(playlist_.begin(), playlist_.end(), full_path) == playlist_.end()) {
        playlist_.push_back(full_path);
        save_playlist();
        Logger::info("已添加媒体到播放列表: " + full_path);
        return true;
    }
    return false;
}

std::vector<std::string> MediaManager::get_playlist() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return playlist_;
}

std::vector<std::string> MediaManager::get_history() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return history_;
}

bool MediaManager::add_to_history(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string full_path = media_dir_ + filename;
    if (fs::exists(full_path)) {
        history_.push_back(full_path);
        if (history_.size() > 100) history_.erase(history_.begin());
        save_history();
        Logger::info("已添加到历史记录: " + full_path);
        return true;
    }
    Logger::error("历史记录文件不存在: " + full_path);
    return false;
}

void MediaManager::sort_playlist() {
    std::lock_guard<std::mutex> lock(mutex_);
    // 按文件名排序播放列表
    std::sort(playlist_.begin(), playlist_.end(), [](const std::string& a, const std::string& b) {
        std::string name_a = a.substr(a.find_last_of("/\\") + 1);
        std::string name_b = b.substr(b.find_last_of("/\\") + 1);
        return name_a < name_b;
    });
    save_playlist();
    Logger::info("播放列表已排序");
}

std::string MediaManager::get_media_path(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string full_path = media_dir_ + filename;
    return fs::exists(full_path) ? full_path : "";
}

std::vector<Subtitle> MediaManager::load_subtitles(const std::string& srt_file) const {
    std::vector<Subtitle> subtitles;
    std::ifstream file(srt_file);
    if (!file.is_open()) {
        Logger::error("无法打开字幕文件: " + srt_file);
        return subtitles;
    }

    // 解析SRT字幕
    std::string line;
    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        if (line.empty()) continue;

        bool is_number;
        try {
            std::stoi(line);
            is_number = true;
        } catch (...) {
            is_number = false;
        }
        if (is_number) continue;

        if (line.find("-->") != std::string::npos) {
            Subtitle subtitle;
            auto parts = line.substr(0, line.find(" --> "));
            auto end_parts = line.substr(line.find(" --> ") + 5);
            auto parse_time = [](const std::string& time_str) -> int64_t {
                int h, m, s, ms;
                sscanf(time_str.c_str(), "%d:%d:%d,%d", &h, &m, &s, &ms);
                return h * 3600000 + m * 60000 + s * 1000 + ms;
            };
            subtitle.startTime = parse_time(parts);
            subtitle.endTime = parse_time(end_parts);

            std::string text;
            while (std::getline(file, line) && !line.empty()) {
                text += line + "\n";
            }
            subtitle.text = text;
            subtitles.push_back(subtitle);
        }
    }
    file.close();
    Logger::info("已加载字幕: " + srt_file);
    return subtitles;
}

bool MediaManager::is_valid_media(const std::string& filename) const {
    // 验证媒体文件格式
    std::string ext = filename.substr(filename.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == "mp4" || ext == "mkv" || ext == "avi" || ext == "mp3" || ext == "wav";
}

void MediaManager::save_playlist() {
    std::ofstream file(data_dir_ + "playList.dat");
    for (const auto& item : playlist_) {
        file << item << "\n";
    }
    file.close();
}

void MediaManager::load_playlist() {
    std::ifstream file(data_dir_ + "playList.dat");
    std::string line;
    while (std::getline(file, line)) {
        if (fs::exists(line)) playlist_.push_back(line);
    }
    file.close();
}

void MediaManager::save_history() {
    std::ofstream file(data_dir_ + "history.dat");
    for (const auto& item : history_) {
        file << item << "\n";
    }
    file.close();
}

void MediaManager::load_history() {
    std::ifstream file(data_dir_ + "history.dat");
    std::string line;
    while (std::getline(file, line)) {
        if (fs::exists(line)) history_.push_back(line);
    }
    file.close();
}