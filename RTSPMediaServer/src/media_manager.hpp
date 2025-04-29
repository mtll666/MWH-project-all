#ifndef MEDIA_MANAGER_HPP
#define MEDIA_MANAGER_HPP

#include <string>
#include <vector>
#include <mutex>
#include "logger.hpp"

// 字幕结构体
struct Subtitle {
    int64_t startTime; // 字幕开始时间（毫秒）
    int64_t endTime;   // 字幕结束时间（毫秒）
    std::string text;  // 字幕文本
};

// 媒体管理类，处理播放列表、历史记录和字幕
class MediaManager {
public:
    // 构造函数，初始化媒体和数据目录
    MediaManager(const std::string& media_dir, const std::string& data_dir);

    // 添加媒体文件到播放列表
    bool add_media(const std::string& filename);

    // 获取播放列表
    std::vector<std::string> get_playlist() const;

    // 获取历史记录
    std::vector<std::string> get_history() const;

    // 添加到历史记录
    bool add_to_history(const std::string& filename);

    // 排序播放列表
    void sort_playlist();

    // 获取媒体文件路径
    std::string get_media_path(const std::string& filename) const;

    // 加载SRT字幕
    std::vector<Subtitle> load_subtitles(const std::string& srt_file) const;

    // 验证媒体文件格式
    bool is_valid_media(const std::string& filename) const;

private:
    std::string media_dir_; // 媒体目录
    std::string data_dir_;  // 数据目录
    std::vector<std::string> playlist_; // 播放列表
    std::vector<std::string> history_;  // 历史记录
    mutable std::mutex mutex_; // 互斥锁
    void save_playlist(); // 保存播放列表
    void save_history();  // 保存历史记录
    void load_playlist(); // 加载播放列表
    void load_history();  // 加载历史记录
};

#endif // MEDIA_MANAGER_HPP