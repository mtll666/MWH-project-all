#include "reversedecode.h"
#include "controller.h"
#include <QDebug>
#include <QImage>
#include <QFileInfo>
#include <mutex>
#include <vector>
#include <deque>
extern "C" {
#include <libavutil/imgutils.h>
}

// 构造函数，初始化控制器
ReverseDecode::ReverseDecode(Controller *ctrl)
    : ctrl(ctrl)
{
    qDebug() << "ReverseDecode 初始化";
}

// 析构函数，释放 FFmpeg 资源
ReverseDecode::~ReverseDecode()
{
    qDebug() << "ReverseDecode 开始释放资源";
    if (img_convert_ctx) {
        sws_freeContext(img_convert_ctx);
        img_convert_ctx = nullptr;
    }
    if (out_buffer_rgb) {
        av_free(out_buffer_rgb);
        out_buffer_rgb = nullptr;
    }
    if (RGB24_pFrame) {
        av_frame_free(&RGB24_pFrame);
        RGB24_pFrame = nullptr;
    }
    if (pFrame) {
        av_frame_free(&pFrame);
        pFrame = nullptr;
    }
    if (video_dec_ctx) {
        avcodec_free_context(&video_dec_ctx);
        video_dec_ctx = nullptr;
    }
    if (format_ctx) {
        avformat_close_input(&format_ctx);
        format_ctx = nullptr;
    }
    qDebug() << "ReverseDecode 资源释放完成";
}

// 加载视频文件并初始化解码器
int ReverseDecode::loadFile(QString filename)
{
    qDebug() << "加载视频文件: " << filename;
    QFileInfo fileInfo(filename);
    if (!fileInfo.exists()) {
        qDebug() << "文件不存在: " << filename;
        return -1;
    }
    format_ctx = avformat_alloc_context();
    if (!format_ctx) {
        qDebug() << "无法分配格式上下文";
        return -1;
    }
    if (avformat_open_input(&format_ctx, filename.toStdString().c_str(), nullptr, nullptr) < 0) {
        qDebug() << "无法打开文件: " << filename;
        avformat_free_context(format_ctx);
        format_ctx = nullptr;
        return -1;
    }
    if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
        qDebug() << "无法获取流信息: " << filename;
        avformat_close_input(&format_ctx);
        format_ctx = nullptr;
        return -1;
    }
    video_stream_index = -1;
    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }
    if (video_stream_index == -1) {
        qDebug() << "未找到视频流: " << filename;
        avformat_close_input(&format_ctx);
        format_ctx = nullptr;
        return -1;
    }
    const AVCodec *dec = avcodec_find_decoder(format_ctx->streams[video_stream_index]->codecpar->codec_id);
    if (!dec) {
        qDebug() << "未找到解码器: " << filename;
        avformat_close_input(&format_ctx);
        format_ctx = nullptr;
        return -1;
    }
    video_dec_ctx = avcodec_alloc_context3(dec);
    if (!video_dec_ctx) {
        qDebug() << "无法分配解码上下文: " << filename;
        avformat_close_input(&format_ctx);
        format_ctx = nullptr;
        return -1;
    }
    if (avcodec_parameters_to_context(video_dec_ctx, format_ctx->streams[video_stream_index]->codecpar) < 0) {
        qDebug() << "无法设置解码器参数: " << filename;
        avcodec_free_context(&video_dec_ctx);
        avformat_close_input(&format_ctx);
        video_dec_ctx = nullptr;
        format_ctx = nullptr;
        return -1;
    }
    if (avcodec_open2(video_dec_ctx, dec, nullptr) < 0) {
        qDebug() << "无法打开解码器: " << filename;
        avcodec_free_context(&video_dec_ctx);
        avformat_close_input(&format_ctx);
        video_dec_ctx = nullptr;
        format_ctx = nullptr;
        return -1;
    }
    video_width = video_dec_ctx->width;
    video_height = video_dec_ctx->height;
    if (video_width <= 0 || video_height <= 0) {
        qDebug() << "无效的视频分辨率: width=" << video_width << ", height=" << video_height;
        avcodec_free_context(&video_dec_ctx);
        avformat_close_input(&format_ctx);
        video_dec_ctx = nullptr;
        format_ctx = nullptr;
        return -1;
    }
    duration = format_ctx->duration;
    if (duration <= 0) {
        qDebug() << "无效的视频时长: " << duration;
        avcodec_free_context(&video_dec_ctx);
        avformat_close_input(&format_ctx);
        video_dec_ctx = nullptr;
        format_ctx = nullptr;
        return -1;
    }
    pFrame = av_frame_alloc();
    if (!pFrame) {
        qDebug() << "无法分配帧: " << filename;
        avcodec_free_context(&video_dec_ctx);
        avformat_close_input(&format_ctx);
        video_dec_ctx = nullptr;
        format_ctx = nullptr;
        return -1;
    }
    RGB24_pFrame = av_frame_alloc();
    if (!RGB24_pFrame) {
        qDebug() << "无法分配 RGB 帧: " << filename;
        av_frame_free(&pFrame);
        avcodec_free_context(&video_dec_ctx);
        avformat_close_input(&format_ctx);
        pFrame = nullptr;
        video_dec_ctx = nullptr;
        format_ctx = nullptr;
        return -1;
    }
    img_convert_ctx = sws_getContext(
        video_width, video_height, video_dec_ctx->pix_fmt,
        video_width, video_height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr
        );
    if (!img_convert_ctx) {
        qDebug() << "无法创建图像转换上下文: " << filename;
        av_frame_free(&RGB24_pFrame);
        av_frame_free(&pFrame);
        avcodec_free_context(&video_dec_ctx);
        avformat_close_input(&format_ctx);
        RGB24_pFrame = nullptr;
        pFrame = nullptr;
        video_dec_ctx = nullptr;
        format_ctx = nullptr;
        return -1;
    }
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, video_width, video_height, 1);
    if (numBytes <= 0) {
        qDebug() << "无效的缓冲区大小: " << filename;
        sws_freeContext(img_convert_ctx);
        av_frame_free(&RGB24_pFrame);
        av_frame_free(&pFrame);
        avcodec_free_context(&video_dec_ctx);
        avformat_close_input(&format_ctx);
        img_convert_ctx = nullptr;
        RGB24_pFrame = nullptr;
        pFrame = nullptr;
        video_dec_ctx = nullptr;
        format_ctx = nullptr;
        return -1;
    }
    out_buffer_rgb = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
    if (!out_buffer_rgb) {
        qDebug() << "无法分配 RGB 缓冲区: " << filename;
        sws_freeContext(img_convert_ctx);
        av_frame_free(&RGB24_pFrame);
        av_frame_free(&pFrame);
        avcodec_free_context(&video_dec_ctx);
        avformat_close_input(&format_ctx);
        img_convert_ctx = nullptr;
        RGB24_pFrame = nullptr;
        pFrame = nullptr;
        video_dec_ctx = nullptr;
        format_ctx = nullptr;
        return -1;
    }
    int ret = av_image_fill_arrays(
        RGB24_pFrame->data, RGB24_pFrame->linesize, out_buffer_rgb,
        AV_PIX_FMT_RGB24, video_width, video_height, 1
        );
    if (ret < 0) {
        qDebug() << "无法填充图像数组: " << filename;
        av_free(out_buffer_rgb);
        sws_freeContext(img_convert_ctx);
        av_frame_free(&RGB24_pFrame);
        av_frame_free(&pFrame);
        avcodec_free_context(&video_dec_ctx);
        avformat_close_input(&format_ctx);
        out_buffer_rgb = nullptr;
        img_convert_ctx = nullptr;
        RGB24_pFrame = nullptr;
        pFrame = nullptr;
        video_dec_ctx = nullptr;
        format_ctx = nullptr;
        return -1;
    }
    qDebug() << "视频初始化成功: " << filename;
    return 0;
}

// 线程运行，解码视频帧并存储
void ReverseDecode::run()
{
    qDebug() << "开始解码线程";
    AVPacket packet;
    qint64 cur = duration;
    int i = 0;
    while (!ctrl->isQuit) {
        qDebug() << "解码循环, cur=" << cur << " i=" << i;
        i++;
        if (ctrl->isDecoderSeek) {
            std::unique_lock<std::mutex> lk(ctrl->seekMutex);
            if (avformat_seek_file(format_ctx, video_stream_index, INT64_MIN, ctrl->seekPos, INT64_MAX, 0) < 0) {
                qDebug() << "跳转失败: seekPos=" << ctrl->seekPos;
            } else {
                avcodec_flush_buffers(video_dec_ctx);
                cur = ctrl->seekPos;
                qDebug() << "执行跳转: " << cur;
            }
            ctrl->isDecoderSeek = false;
            lk.unlock();
        }
        std::vector<IMAGE_FRAME> V;
        while (av_read_frame(format_ctx, &packet) >= 0) {
            if (packet.stream_index == video_stream_index) {
                if (avcodec_send_packet(video_dec_ctx, &packet) >= 0) {
                    while (avcodec_receive_frame(video_dec_ctx, pFrame) >= 0) {
                        if (!pFrame->data[0]) {
                            qDebug() << "无效帧数据";
                            continue;
                        }
                        sws_scale(
                            img_convert_ctx,
                            pFrame->data, pFrame->linesize,
                            0, video_height,
                            RGB24_pFrame->data, RGB24_pFrame->linesize
                            );
                        QImage img(out_buffer_rgb, video_width, video_height, QImage::Format_RGB888);
                        IMAGE_FRAME frame;
                        frame.pts = pFrame->pts;
                        frame.image = img.copy();
                        V.push_back(frame);
                        qDebug() << "解码帧: pts=" << frame.pts;
                    }
                }
            }
            av_packet_unref(&packet);
            if (V.size() >= 100 || cur <= 0) break;
        }
        std::unique_lock<std::mutex> lk(ctrl->queueMutex);
        for (int j = V.size() - 1; j >= 0; j--) {
            if (ctrl->queue.size() >= 1000) {
                ctrl->queue.pop_front();
                qDebug() << "队列已满，移除最早帧";
            }
            ctrl->queue.push_back(V[j]);
            qDebug() << "添加帧到队列: pts=" << V[j].pts;
        }
        lk.unlock();
        if (cur <= 0) {
            qDebug() << "解码完成，cur=" << cur;
            break;
        }
        cur -= 1000000;
        if (cur < 0) cur = 0;
        if (avformat_seek_file(format_ctx, video_stream_index, INT64_MIN, cur, INT64_MAX, 0) < 0) {
            qDebug() << "跳转失败: cur=" << cur;
        } else {
            avcodec_flush_buffers(video_dec_ctx);
            qDebug() << "跳转到: " << cur;
        }
    }
    qDebug() << "解码线程结束";
}

// 退出解码线程
void ReverseDecode::quit()
{
    qDebug() << "退出解码线程";
    ctrl->isQuit = true;
}
