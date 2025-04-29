// 实现视频帧定位功能
#include "seekFrame.h"
#include <QDebug>

// 构造函数，初始化定位参数
SeekFrame::SeekFrame(QString fileName, int maxFindTimes, double minDistance)
{
    std::string fileName_str = fileName.toStdString();
    const char* fileName_const = fileName_str.c_str();
    init(fileName_const);
    this->maxFindTimes = maxFindTimes;
    this->minDistance = minDistance;
}

// 初始化 FFmpeg 上下文
void SeekFrame::init(const char* fileName) {
    formatContext = avformat_alloc_context();
    const AVCodec* codec = nullptr;
    AVCodecParameters* codecParameters = nullptr;
    videoStreamIndex = -1;
    if (avformat_open_input(&formatContext, fileName, nullptr, nullptr) != 0) {
        qDebug() << "无法打开文件";
        return;
    }
    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        qDebug() << "无法获取流信息";
        avformat_close_input(&formatContext);
        return;
    }
    for (int i = 0; i < (int)formatContext->nb_streams; i++) {
        AVCodecParameters* localCodecParameters = formatContext->streams[i]->codecpar;
        const AVCodec* localCodec = avcodec_find_decoder(localCodecParameters->codec_id);
        if (localCodec == nullptr) {
            qDebug() << "不支持的编码器，流" << i;
            continue;
        }
        if (localCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (videoStreamIndex == -1) {
                videoStreamIndex = i;
                codec = localCodec;
                codecParameters = localCodecParameters;
                video_width = localCodecParameters->width;
                video_height = localCodecParameters->height;
            }
        }
    }
    if (videoStreamIndex == -1) {
        qDebug() << "未找到视频流";
        avformat_close_input(&formatContext);
        return;
    }
    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        qDebug() << "无法分配编解码上下文";
        avformat_close_input(&formatContext);
        return;
    }
    if (avcodec_parameters_to_context(codecContext, codecParameters) < 0) {
        qDebug() << "无法复制编解码参数";
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return;
    }
    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "extra_hw_frames", "256", 0);
    if (avcodec_open2(codecContext, codec, &opts) < 0) {
        qDebug() << "无法打开硬件解码，尝试软件解码";
        av_dict_free(&opts);
        avcodec_free_context(&codecContext);
        codecContext = avcodec_alloc_context3(codec);
        if (avcodec_parameters_to_context(codecContext, codecParameters) < 0 || avcodec_open2(codecContext, codec, nullptr) < 0) {
            qDebug() << "无法打开软件解码";
            avcodec_free_context(&codecContext);
            avformat_close_input(&formatContext);
            return;
        }
    }
    av_dict_free(&opts);
    sws_ctx = sws_getContext(
        codecContext->width,
        codecContext->height,
        codecContext->pix_fmt,
        codecContext->width,
        codecContext->height,
        AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        nullptr,
        nullptr,
        nullptr
        );
    if (!sws_ctx) {
        qDebug() << "无法创建转换上下文";
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return;
    }
    avd = av_q2d(formatContext->streams[videoStreamIndex]->time_base);
}

// 获取指定时间的视频帧
AVFrame* SeekFrame::getFrame(int wanted_time) {
    int response = 0;
    int howManypacketsToProcess = this->maxFindTimes;
    AVFrame* frame = av_frame_alloc();
    AVPacket* packet = av_packet_alloc();
    AVFrame* pFrameRGB = av_frame_alloc();
    if (!frame || !packet || !pFrameRGB) {
        qDebug() << "无法分配帧或包";
        av_frame_free(&frame);
        av_frame_free(&pFrameRGB);
        av_packet_free(&packet);
        return nullptr;
    }
    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, codecContext->width, codecContext->height, 1);
    unsigned char* frame_buffer = (uint8_t*)av_malloc(num_bytes);
    if (!frame_buffer) {
        qDebug() << "无法分配帧缓冲区";
        av_frame_free(&frame);
        av_frame_free(&pFrameRGB);
        av_packet_free(&packet);
        return nullptr;
    }
    response = av_image_fill_arrays(
        pFrameRGB->data,
        pFrameRGB->linesize,
        frame_buffer,
        AV_PIX_FMT_RGB24,
        codecContext->width,
        codecContext->height,
        1
        );
    if (response < 0) {
        qDebug() << "填充图像数组失败，响应 =" << response;
        av_free(frame_buffer);
        av_frame_free(&frame);
        av_frame_free(&pFrameRGB);
        av_packet_free(&packet);
        return nullptr;
    }
    pFrameRGB->width = codecContext->width;
    pFrameRGB->height = codecContext->height;
    int64_t want_ts2 = int64_t(wanted_time / avd);
    qDebug() << "目标时间" << wanted_time << "目标时间戳" << want_ts2;
    if (av_seek_frame(formatContext, videoStreamIndex, want_ts2, AVSEEK_FLAG_BACKWARD) < 0) {
        qDebug() << "定位帧失败";
        av_free(frame_buffer);
        av_frame_free(&frame);
        av_frame_free(&pFrameRGB);
        av_packet_free(&packet);
        return nullptr;
    }
    avcodec_flush_buffers(codecContext);
    bool flag = false;
    while (av_read_frame(formatContext, packet) >= 0 && !flag) {
        if (packet->stream_index == videoStreamIndex) {
            response = avcodec_send_packet(codecContext, packet);
            if (response < 0) {
                qDebug() << "发送包到解码器错误:" << response;
                av_packet_unref(packet);
                break;
            }
            while (response >= 0) {
                response = avcodec_receive_frame(codecContext, frame);
                if (response == AVERROR(EAGAIN)) {
                    break;
                }
                if (response == AVERROR_EOF) {
                    qDebug() << "解码器到达文件末尾";
                    break;
                }
                if (response < 0) {
                    qDebug() << "接收帧错误:" << response;
                    break;
                }
                double frame_time = (double)avd * frame->best_effort_timestamp;
                if (abs(frame_time - (double)wanted_time) < minDistance) {
                    response = sws_scale(
                        sws_ctx,
                        (unsigned char const* const*)(frame->data),
                        frame->linesize,
                        0,
                        codecContext->height,
                        pFrameRGB->data,
                        pFrameRGB->linesize
                        );
                    if (response < 0) {
                        qDebug() << "图像转换失败:" << response;
                    } else {
                        qDebug() << "最佳时间:" << frame_time << "帧时间戳:" << frame->best_effort_timestamp;
                        av_packet_unref(packet);
                        av_frame_free(&frame);
                        return pFrameRGB;
                    }
                }
                av_frame_unref(frame);
            }
            if (--howManypacketsToProcess <= 0) {
                break;
            }
        }
        av_packet_unref(packet);
    }
    av_free(frame_buffer);
    av_frame_free(&frame);
    av_frame_free(&pFrameRGB);
    av_packet_free(&packet);
    return nullptr;
}

// 析构函数，释放资源
SeekFrame::~SeekFrame() {
    if (sws_ctx) sws_freeContext(sws_ctx);
    if (codecContext) avcodec_free_context(&codecContext);
    if (formatContext) {
        avformat_close_input(&formatContext);
        avformat_free_context(formatContext);
    }
}
