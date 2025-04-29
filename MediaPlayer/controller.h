#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <mutex>
#include <deque>
#include <QImage>
#include <QDebug>

// 前向声明
class ReverseDecode;
class ReverseDisplay;
class GetAudio;
class VideoFrameDisplay;

// 视频帧结构
struct IMAGE_FRAME {
    qint64 pts;
    QImage image;
};

// 控制器类，管理倒放和波形图
class Controller {
public:
    Controller();
    ~Controller();
    void init();
    void print();
    void setVideoPath(const QString &path);
    bool startReversePlay(const QString &fileName, VideoFrameDisplay *display);
    void stopReversePlay();
    void seekReverse(qint64 seekPos);
    void setPlaybackRate(float ratio, bool isReverse);
    void initWaveForm(const QString &fileName);
    void deleteWaveForm();
    void resizeWaveForm(int height);
    void updateWaveForm(qint64 progress);
    void setPlaying(bool playing, bool isReverse);
    ReverseDecode* getReverseDecoder() const { return reverseDecoder; }
    ReverseDisplay* getReverseDisplayer() const { return reverseDisplayer; }

    qint64 seekPos;
    bool isQuit;
    bool isDecoderSeek;
    bool isDisplayerSeek;
    std::mutex seekMutex;
    std::mutex quitMutex;
    std::mutex queueMutex;
    std::deque<IMAGE_FRAME> queue;

private:
    QString videoPath;
    ReverseDecode *reverseDecoder = nullptr;
    ReverseDisplay *reverseDisplayer = nullptr;
    GetAudio *audioGetFrame = nullptr;
    bool isPlaying = false;
    bool isReverseMode = false;
};

#endif // CONTROLLER_H
