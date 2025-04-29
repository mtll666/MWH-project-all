#include "rtsp_server.hpp"
#include "thread_pool.hpp"
#include "config.hpp"
#include "logger.hpp"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <cstring>

// 设置套接字为非阻塞模式
void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    // 加载配置文件
    Config config("config.ini");
    int port = config.get_int("server.port");
    std::string media_dir = config.get("server.media_dir");
    std::string data_dir = config.get("server.data_dir");
    int max_threads = config.get_int("server.max_threads");

    // 初始化媒体管理器和线程池
    MediaManager media_manager(media_dir, data_dir);
    ThreadPool pool(max_threads);
    RTSPServer server("rtsp://0.0.0.0:8554", port, media_manager);

    // 创建epoll实例
    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        Logger::error("无法创建epoll实例");
        return 1;
    }

    // 启动RTSP服务器
    server.start([&](int client_fd, const std::string& filename) {
        set_nonblocking(client_fd);
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET; // 边缘触发模式
        ev.data.fd = client_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) < 0) {
            Logger::error("无法将客户端添加到epoll");
            close(client_fd);
            return;
        }
        Logger::info("客户端连接: " + std::to_string(client_fd) + ", 请求: " + filename);
        // 将流传输任务添加到线程池
        pool.enqueue([&server, client_fd, filename] {
            server.stream_media(client_fd, filename);
        });
    });

    // epoll事件循环
    std::vector<struct epoll_event> events(100);
    while (true) {
        int nfds = epoll_wait(epoll_fd, events.data(), 100, -1);
        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;
            if (events[i].events & EPOLLIN) {
                char buffer[1024];
                int bytes_read = read(fd, buffer, sizeof(buffer) - 1);
                if (bytes_read <= 0) {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                    close(fd);
                    Logger::info("客户端断开: " + std::to_string(fd));
                }
            } else {
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                close(fd);
                Logger::info("客户端错误，断开: " + std::to_string(fd));
            }
        }
    }

    close(epoll_fd);
    return 0;
}