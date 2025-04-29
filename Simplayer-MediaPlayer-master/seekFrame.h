#ifndef SEEKFRAME_H
#define SEEKFRAME_H

#include <QString>
#include <string>
#include <iostream>
#include "utils.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

class SeekFrame
{
public:
    SeekFrame(QString fileName, int maxFindTimes, double minDistance);
    ~SeekFrame();
    void init(const char* fileName); // 初始化媒体上下文和解码器
    AVFrame* getFrame(int wanted_time); // 获取指定时间的帧
    AVFormatContext* formatContext = nullptr; // 媒体上下文
    AVCodecContext* codecContext = nullptr; // 解码上下文
    int videoStreamIndex; // 视频流索引
    int maxFindTimes; // 最大查找次数
    double minDistance; // 时间距离阈值
    double avd; // 时间基
    int video_width; // 视频宽度
    int video_height; // 视频高度
    SwsContext* sws_ctx = nullptr; // 图像转换上下文
};

#endif // SEEKFRAME_H
