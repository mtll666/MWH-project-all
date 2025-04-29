#ifndef REVERSEDISPLAY_H
#define REVERSEDISPLAY_H

#include <QThread>
#include "Widget/VideoFrameDisplay.h"
extern "C" {
#include <libavformat/avformat.h>
}

// 前向声明
class Controller;

// 倒放显示器类，处理视频帧显示
class ReverseDisplay : public QThread
{
    Q_OBJECT
public:
    explicit ReverseDisplay(Controller *ctrl, VideoFrameDisplay *display, AVFormatContext *fmt_ctx, int stream_index);
    ~ReverseDisplay();
    void pauseThread();
    void resumeThread();
    void quit();
    bool pause_ = false;

signals:
    void SendOneFrame(QImage);
    void SendTime(qint64);
    void SendSecond(double);

protected:
    void run() override;

private:
    Controller *ctrl;
    VideoFrameDisplay *display;
    AVFormatContext *format_ctx;
    int video_stream_index;
    std::mutex pauseMutex;
    std::condition_variable pauseCond;
};

#endif // REVERSEDISPLAY_H
