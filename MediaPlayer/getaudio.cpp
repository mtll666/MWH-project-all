#include "getaudio.h"
#include <QFile>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QApplication>

// 构造函数，初始化音频提取
GetAudio::GetAudio(const QString &filename, const QString &pcmPath)
    : pcmFilePath(pcmPath)
{
    qDebug() << "GetAudio 构造: input=" << filename << ", output=" << pcmPath;
    init();

    // 检查 PCM 文件目录
    QFileInfo pcmInfo(pcmPath);
    QDir pcmDir = pcmInfo.dir();
    if (!pcmDir.exists()) {
        if (!pcmDir.mkpath(".")) {
            qDebug() << "无法创建 PCM 目录: " << pcmDir.path();
            return;
        }
    }

    // 检查输入文件
    QFileInfo inputInfo(filename);
    if (!inputInfo.exists() || !inputInfo.isReadable()) {
        qDebug() << "输入文件不可访问: " << filename;
        return;
    }

    fmtCtx = avformat_alloc_context();
    if (!fmtCtx) {
        qDebug() << "无法分配格式上下文";
        return;
    }
    if (avformat_open_input(&fmtCtx, filename.toStdString().c_str(), nullptr, nullptr) < 0) {
        qDebug() << "无法打开音频文件: " << filename;
        avformat_free_context(fmtCtx);
        fmtCtx = nullptr;
        return;
    }
    if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
        qDebug() << "无法获取音频流信息: " << filename;
        avformat_close_input(&fmtCtx);
        fmtCtx = nullptr;
        return;
    }
    audioStreamIndex = -1;
    for (unsigned int i = 0; i < fmtCtx->nb_streams; ++i) {
        if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
            break;
        }
    }
    if (audioStreamIndex == -1) {
        qDebug() << "未找到音频流: " << filename;
        avformat_close_input(&fmtCtx);
        fmtCtx = nullptr;
        return;
    }
    const AVCodec *codec = avcodec_find_decoder(fmtCtx->streams[audioStreamIndex]->codecpar->codec_id);
    if (!codec) {
        qDebug() << "未找到音频解码器: " << filename;
        avformat_close_input(&fmtCtx);
        fmtCtx = nullptr;
        return;
    }
    codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) {
        qDebug() << "无法分配解码上下文: " << filename;
        avformat_close_input(&fmtCtx);
        fmtCtx = nullptr;
        return;
    }
    if (avcodec_parameters_to_context(codecCtx, fmtCtx->streams[audioStreamIndex]->codecpar) < 0) {
        qDebug() << "无法设置解码器参数: " << filename;
        avcodec_free_context(&codecCtx);
        avformat_close_input(&fmtCtx);
        codecCtx = nullptr;
        fmtCtx = nullptr;
        return;
    }
    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        qDebug() << "无法打开解码器: " << filename;
        avcodec_free_context(&codecCtx);
        avformat_close_input(&fmtCtx);
        codecCtx = nullptr;
        fmtCtx = nullptr;
        return;
    }
    swrCtx = nullptr;
    AVChannelLayout out_layout;
    av_channel_layout_default(&out_layout, 1); // 单声道
    AVChannelLayout in_layout;
    av_channel_layout_copy(&in_layout, &codecCtx->ch_layout);
    if (in_layout.nb_channels == 0) {
        av_channel_layout_default(&in_layout, codecCtx->ch_layout.nb_channels ? codecCtx->ch_layout.nb_channels : 2);
    }
    if (swr_alloc_set_opts2(&swrCtx, &out_layout, AV_SAMPLE_FMT_S16, 44100,
                            &in_layout, codecCtx->sample_fmt, codecCtx->sample_rate,
                            0, nullptr) < 0 || swr_init(swrCtx) < 0) {
        qDebug() << "无法初始化重采样上下文: " << filename;
        av_channel_layout_uninit(&in_layout);
        av_channel_layout_uninit(&out_layout);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&fmtCtx);
        swrCtx = nullptr;
        codecCtx = nullptr;
        fmtCtx = nullptr;
        return;
    }
    av_channel_layout_uninit(&in_layout);
    av_channel_layout_uninit(&out_layout);

    // 处理音频并写入 PCM 文件
    if (!processAudioToPcm()) {
        qDebug() << "音频处理失败: " << filename;
        swr_free(&swrCtx);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&fmtCtx);
        swrCtx = nullptr;
        codecCtx = nullptr;
        fmtCtx = nullptr;
        return;
    }

    qDebug() << "音频初始化成功: " << filename << ", PCM 文件: " << pcmFilePath << ", total_len=" << total_len;
}

// 处理音频并写入 PCM 文件
bool GetAudio::processAudioToPcm()
{
    qDebug() << "开始处理音频到 PCM 文件: " << pcmFilePath;
    QFile pcmFile(pcmFilePath);
    if (!pcmFile.open(QIODevice::WriteOnly)) {
        qDebug() << "无法创建 PCM 文件: " << pcmFilePath;
        return false;
    }

    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    if (!packet || !frame) {
        qDebug() << "无法分配帧或包: packet=" << packet << ", frame=" << frame;
        if (packet) av_packet_free(&packet);
        if (frame) av_frame_free(&frame);
        pcmFile.close();
        return false;
    }

    bool success = true;
    while (av_read_frame(fmtCtx, packet) >= 0) {
        if (packet->stream_index == audioStreamIndex) {
            int ret = avcodec_send_packet(codecCtx, packet);
            if (ret < 0) {
                qDebug() << "发送包到解码器失败: ret=" << ret;
                success = false;
                break;
            }
            while (ret >= 0) {
                ret = avcodec_receive_frame(codecCtx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                }
                if (ret < 0) {
                    qDebug() << "接收帧失败: ret=" << ret;
                    success = false;
                    break;
                }
                // 分配输出缓冲区
                int out_samples_max = swr_get_out_samples(swrCtx, frame->nb_samples);
                if (out_samples_max < 0) {
                    qDebug() << "无法获取输出样本数: " << out_samples_max;
                    success = false;
                    break;
                }
                std::vector<uint8_t> out_buffer(out_samples_max * 2); // 单声道 16-bit
                uint8_t *out_ptr = out_buffer.data();
                int out_samples = swr_convert(swrCtx, &out_ptr, out_samples_max,
                                              (const uint8_t **)frame->data, frame->nb_samples);
                if (out_samples > 0) {
                    pcmFile.write((const char *)out_buffer.data(), out_samples * 2);
                    total_len += out_samples;
                } else if (out_samples < 0) {
                    qDebug() << "重采样失败: out_samples=" << out_samples;
                    success = false;
                    break;
                }
                av_frame_unref(frame);
            }
        }
        av_packet_unref(packet);
        if (!success) break;
    }

    pcmFile.close();
    av_packet_free(&packet);
    av_frame_free(&frame);
    qDebug() << "音频处理完成: success=" << success << ", total_len=" << total_len;
    return success;
}

// 析构函数，释放 FFmpeg 资源
GetAudio::~GetAudio()
{
    qDebug() << "音频析构开始";
    if (swrCtx) {
        swr_free(&swrCtx);
        swrCtx = nullptr;
    }
    if (codecCtx) {
        avcodec_free_context(&codecCtx);
        codecCtx = nullptr;
    }
    if (fmtCtx) {
        avformat_close_input(&fmtCtx);
        fmtCtx = nullptr;
    }
    qDebug() << "音频析构完成";
}

// 初始化 FFmpeg 资源
void GetAudio::init()
{
    fmtCtx = nullptr;
    codecCtx = nullptr;
    swrCtx = nullptr;
    audioStreamIndex = -1;
    total_len = 0;
}

// 加载指定时间的音频数据
long GetAudio::loadAudio(long second)
{
    qDebug() << "loadAudio: second=" << second;
    if (total_len <= 0) {
        qDebug() << "无效的 total_len: " << total_len;
        return 0;
    }
    QFile pcmFile(pcmFilePath);
    if (!pcmFile.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开 PCM 文件: " << pcmFilePath;
        return 0;
    }
    long offset = second * 44100 * 2; // 44.1kHz 单声道 16位
    if (offset >= pcmFile.size()) {
        qDebug() << "偏移超出文件大小: offset=" << offset << ", size=" << pcmFile.size();
        pcmFile.close();
        return total_len / 44100;
    }
    if (!pcmFile.seek(offset)) {
        qDebug() << "无法定位 PCM 文件: offset=" << offset;
        pcmFile.close();
        return 0;
    }
    qDebug() << "加载音频数据成功: second=" << second << ", offset=" << offset;
    pcmFile.close();
    return second;
}
