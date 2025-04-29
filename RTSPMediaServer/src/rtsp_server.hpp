#ifndef RTSP_SERVER_HPP
#define RTSP_SERVER_HPP

#include <string>
#include <thread>
#include <memory>
#include <functional>
#include "media_manager.hpp"
#include "logger.hpp"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

// RTSP服务器类，处理客户端流媒体请求
class RTSPServer {
public:
    // 构造函数，初始化RTSP服务器
    RTSPServer(const std::string& rtsp_url, int port, MediaManager& media_manager);

    // 析构函数，清理资源
    ~RTSPServer();

    // 启动服务器，接受客户端连接
    void start(std::function<void(int, const std::string&)> on_client_connect);

    // 停止服务器
    void stop();

    // 流式传输媒体
    void stream_media(int client_fd, const std::string& filename, const std::string& subtitle_file = "");

private:
    std::string rtsp_url_; // RTSP URL
    int port_; // 监听端口
    MediaManager& media_manager_; // 媒体管理器引用
    std::thread server_thread_; // 服务器线程
    bool running_; // 运行状态
};

#endif // RTSP_SERVER_HPP