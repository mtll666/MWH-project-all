#include "reversedisplay.h"
#include "controller.h"
#include <QDebug>
#include <QTime>
#include <mutex>
#include <condition_variable>
extern "C" {
#include <libavutil/rational.h>
}

// 构造函数，初始化控制器、显示器和 FFmpeg 上下文
ReverseDisplay::ReverseDisplay(Controller *ctrl, VideoFrameDisplay *display, AVFormatContext *fmt_ctx, int stream_index)
    : ctrl(ctrl), display(display), format_ctx(fmt_ctx), video_stream_index(stream_index)
{
    qDebug() << "ReverseDisplay 初始化";
}

// 析构函数
ReverseDisplay::~ReverseDisplay()
{
    qDebug() << "ReverseDisplay 析构";
}

// 线程运行，显示倒放帧
void ReverseDisplay::run()
{
    qDebug() << "开始显示线程";
    QTime lastEmitTime;
    while (!ctrl->isQuit) {
        if (pause_) {
            std::unique_lock<std::mutex> lk(pauseMutex);
            pauseCond.wait(lk, [this] { return !pause_ || ctrl->isQuit; });
            lk.unlock();
            if (ctrl->isQuit) break;
            qDebug() << "显示线程从暂停恢复";
        }
        if (ctrl->isDisplayerSeek) {
            std::unique_lock<std::mutex> lk(ctrl->seekMutex);
            ctrl->isDisplayerSeek = false;
            lk.unlock();
            qDebug() << "显示线程处理跳转";
        }
        std::unique_lock<std::mutex> lk(ctrl->queueMutex);
        if (ctrl->queue.empty()) {
            lk.unlock();
            QThread::msleep(10);
            continue;
        }
        auto V = ctrl->queue.front();
        ctrl->queue.pop_front();
        lk.unlock();
        // 限制信号频率，避免 UI 线程过载
        if (lastEmitTime.isNull() || lastEmitTime.msecsTo(QTime::currentTime()) >= 40) {
            emit SendOneFrame(V.image);
            emit SendTime(V.pts);
            double second = V.pts * av_q2d(format_ctx->streams[video_stream_index]->time_base);
            emit SendSecond(second);
            lastEmitTime = QTime::currentTime();
            qDebug() << "显示帧: pts=" << V.pts << ", second=" << second;
        }
        QThread::msleep(40); // 根据帧率调整
    }
    qDebug() << "显示线程结束";
}

// 暂停线程
void ReverseDisplay::pauseThread()
{
    pause_ = true;
    qDebug() << "暂停显示线程";
}

// 恢复线程
void ReverseDisplay::resumeThread()
{
    pause_ = false;
    pauseCond.notify_one();
    qDebug() << "恢复显示线程";
}

// 退出显示线程
void ReverseDisplay::quit()
{
    ctrl->isQuit = true;
    pause_ = false;
    pauseCond.notify_one();
    qDebug() << "退出显示线程";
}
