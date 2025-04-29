#ifndef GETAUDIO_H
#define GETAUDIO_H

#include <QString>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

// 处理音频提取和 PCM 数据生成
class GetAudio
{
public:
    GetAudio(const QString &filename, const QString &pcmPath);
    ~GetAudio();
    long loadAudio(long second); // 加载指定时间的音频数据
    long total_len = 0; // 音频总长度（样本数）

private:
    void init(); // 初始化 FFmpeg 资源
    bool processAudioToPcm(); // 处理音频并写入 PCM 文件
    AVFormatContext *fmtCtx = nullptr;
    AVCodecContext *codecCtx = nullptr;
    SwrContext *swrCtx = nullptr;
    int audioStreamIndex = -1;
    QString pcmFilePath; // PCM 文件路径
};

#endif // GETAUDIO_H
