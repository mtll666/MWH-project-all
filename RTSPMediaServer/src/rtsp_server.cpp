#include "rtsp_server.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sstream>
#include <regex>

RTSPServer::RTSPServer(const std::string& rtsp_url, int port, MediaManager& media_manager)
    : rtsp_url_(rtsp_url), port_(port), media_manager_(media_manager), running_(false) {
    // 初始化FFmpeg网络模块
    avformat_network_init();
}

RTSPServer::~RTSPServer() {
    stop();
    // 清理FFmpeg网络模块
    avformat_network_deinit();
}

void RTSPServer::start(std::function<void(int, const std::string&)> on_client_connect) {
    running_ = true;
    server_thread_ = std::thread([this, on_client_connect] {
        // 创建TCP套接字
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            Logger::error("无法创建套接字");
            return;
        }

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port_);

        // 绑定端口
        if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            Logger::error("绑定端口失败");
            close(server_fd);
            return;
        }

        // 监听连接
        if (listen(server_fd, 10) < 0) {
            Logger::error("监听失败");
            close(server_fd);
            return;
        }

        Logger::info("RTSP服务器监听端口: " + std::to_string(port_));

        // 接受客户端连接
        while (running_) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd < 0) {
                if (!running_) break;
                Logger::error("接受客户端连接失败");
                continue;
            }

            // 读取RTSP请求
            char buffer[1024];
            int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                std::string request(buffer);
                std::regex rtsp_regex("DESCRIBE rtsp://[^/]+/stream/([^\\s]+) RTSP/1.0");
                std::smatch match;
                std::string filename;
                if (std::regex_search(request, match, rtsp_regex) && match.size() > 1) {
                    filename = match[1].str();
                }
                on_client_connect(client_fd, filename);
            } else {
                close(client_fd);
            }
        }

        close(server_fd);
    });
}

void RTSPServer::stop() {
    running_ = false;
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}

void RTSPServer::stream_media(int client_fd, const std::string& filename, const std::string& subtitle_file) {
    // 获取媒体文件路径
    std::string media_path = media_manager_.get_media_path(filename);
    if (media_path.empty()) {
        Logger::error("媒体文件未找到: " + filename);
        close(client_fd);
        return;
    }

    // 添加到历史记录
    media_manager_.add_to_history(filename);

    AVFormatContext* fmt_ctx = nullptr;
    AVFormatContext* out_ctx = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    int video_stream_idx = -1;

    // 打开输入文件
    if (avformat_open_input(&fmt_ctx, media_path.c_str(), nullptr, nullptr) < 0) {
        Logger::error("无法打开输入文件: " + media_path);
        close(client_fd);
        return;
    }

    // 获取流信息
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        Logger::error("无法获取流信息: " + media_path);
        avformat_close_input(&fmt_ctx);
        close(client_fd);
        return;
    }

    // 查找视频流
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; ++i) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
            break;
        }
    }

    if (video_stream_idx == -1) {
        Logger::error("未找到视频流: " + media_path);
        avformat_close_input(&fmt_ctx);
        close(client_fd);
        return;
    }

    // 创建RTSP输出上下文
    if (avformat_alloc_output_context2(&out_ctx, nullptr, "rtsp", (rtsp_url_ + "/stream/" + filename).c_str()) < 0) {
        Logger::error("无法分配输出上下文");
        avformat_close_input(&fmt_ctx);
        close(client_fd);
        return;
    }

    // 创建输出流
    AVStream* out_stream = avformat_new_stream(out_ctx, nullptr);
    if (!out_stream) {
        Logger::error("无法创建输出流");
        avformat_close_input(&fmt_ctx);
        avformat_free_context(out_ctx);
        close(client_fd);
        return;
    }

    // 配置H.264编码器
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    codec_ctx = avcodec_alloc_context3(codec);
    codec_ctx->width = fmt_ctx->streams[video_stream_idx]->codecpar->width;
    codec_ctx->height = fmt_ctx->streams[video_stream_idx]->codecpar->height;
    codec_ctx->time_base = {1, 25};
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_ctx->bit_rate = 4000000;
    codec_ctx->gop_size = 12;
    codec_ctx->max_b_frames = 2;
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        Logger::error("无法打开编码器");
        avformat_close_input(&fmt_ctx);
        avcodec_free_context(&codec_ctx);
        avformat_free_context(out_ctx);
        close(client_fd);
        return;
    }

    // 设置输出流参数
    if (avcodec_parameters_from_context(out_stream->codecpar, codec_ctx) < 0) {
        Logger::error("无法复制编码参数");
        avformat_close_input(&fmt_ctx);
        avcodec_free_context(&codec_ctx);
        avformat_free_context(out_ctx);
        close(client_fd);
        return;
    }

    // 打开RTSP输出
    if (!(out_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&out_ctx->pb, (rtsp_url_ + "/stream/" + filename).c_str(), AVIO_FLAG_WRITE) < 0) {
            Logger::error("无法打开输出URL");
            avformat_close_input(&fmt_ctx);
            avcodec_free_context(&codec_ctx);
            avformat_free_context(out_ctx);
            close(client_fd);
            return;
        }
    }

    // 写入流头部
    if (avformat_write_header(out_ctx, nullptr) < 0) {
        Logger::error("无法写入流头部");
        avformat_close_input(&fmt_ctx);
        avcodec_free_context(&codec_ctx);
        avformat_free_context(out_ctx);
        close(client_fd);
        return;
    }

    // 传输媒体数据
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = nullptr;
    pkt.size = 0;

    while (true) {
        if (av_read_frame(fmt_ctx, &pkt) < 0) break;
        if (pkt.stream_index == video_stream_idx) {
            pkt.stream_index = 0;
            if (av_interleaved_write_frame(out_ctx, &pkt) < 0) {
                Logger::error("写入帧失败");
                break;
            }
        }
        av_packet_unref(&pkt);
    }

    // 写入流尾部
    av_write_trailer(out_ctx);
    avformat_close_input(&fmt_ctx);
    avcodec_free_context(&codec_ctx);
    if (!(out_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&out_ctx->pb);
    }
    avformat_free_context(out_ctx);
    close(client_fd);
    Logger::info("流传输完成: " + filename);
}