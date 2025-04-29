# RTSP 流媒体服务器

一个高性能的 RTSP 流媒体服务器，使用 C++17、epoll、FFmpeg 3.4.13 和线程池，为 MediaPlayer Qt 客户端设计。

## 功能

- RTSP 流媒体传输（如`rtsp://<server_ip>:8554/stream/<filename>`）。
- 播放列表和历史记录管理，与客户端同步。
- 支持 SRT 字幕（当前记录，未嵌入流）。
- 使用 epoll 和线程池实现高并发。
- 通过`config.ini`配置端口和目录。

## 依赖

- CentOS 7。
- FFmpeg 3.4.13 开发库（`ffmpeg-devel`）。
- g++支持 C++17（gcc 9.3.1）。
- CMake 3.17.5（`cmake3`）。
- 媒体文件（如`test.mp4`）放置在`media/`目录。

## 目录结构
