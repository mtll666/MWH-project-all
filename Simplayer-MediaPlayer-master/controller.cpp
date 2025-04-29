#include "controller.h"
#include <QDir>
#include <QFileInfo>
#include <QApplication>
#include "reversedecode.h"
#include "reversedisplay.h"
#include "getaudio.h"
#include "Widget/VideoFrameDisplay.h"

// 构造函数
Controller::Controller() : seekPos(0), isQuit(false), isDecoderSeek(false), isDisplayerSeek(false) {
    qDebug() << "Controller 初始化";
}

// 析构函数
Controller::~Controller() {
    stopReversePlay();
    deleteWaveForm();
    qDebug() << "Controller 析构";
}

// 初始化
void Controller::init() {
    seekPos = 0;
    isQuit = false;
    isDecoderSeek = false;
    isDisplayerSeek = false;
    queue.clear();
    qDebug() << "Controller 初始化状态";
}

// 打印状态
void Controller::print() {
    qDebug() << "Controller: seekPos=" << seekPos << ", isQuit=" << isQuit;
}

// 设置视频路径
void Controller::setVideoPath(const QString &path) {
    videoPath = path;
}

// 启动倒放
bool Controller::startReversePlay(const QString &fileName, VideoFrameDisplay *display) {
    stopReversePlay();
    reverseDecoder = new ReverseDecode(this);
    if (!reverseDecoder || reverseDecoder->loadFile(fileName) != 0) {
        qDebug() << "倒放解码器初始化失败: " << fileName;
        delete reverseDecoder;
        reverseDecoder = nullptr;
        return false;
    }
    reverseDisplayer = new ReverseDisplay(this, display, reverseDecoder->format_ctx, reverseDecoder->video_stream_index);
    if (!reverseDisplayer) {
        qDebug() << "倒放显示器初始化失败: " << fileName;
        delete reverseDecoder;
        reverseDecoder = nullptr;
        return false;
    }
    reverseDecoder->start();
    reverseDisplayer->start();
    isReverseMode = true;
    qDebug() << "倒放启动成功: " << fileName;
    return true;
}

// 停止倒放
void Controller::stopReversePlay() {
    if (reverseDecoder || reverseDisplayer) {
        seekMutex.lock();
        isDisplayerSeek = true;
        seekMutex.unlock();
        quitMutex.lock();
        isQuit = true;
        quitMutex.unlock();
        if (reverseDecoder) {
            reverseDecoder->quit();
            reverseDecoder->wait();
            delete reverseDecoder;
            reverseDecoder = nullptr;
        }
        if (reverseDisplayer) {
            reverseDisplayer->quit();
            reverseDisplayer->wait();
            delete reverseDisplayer;
            reverseDisplayer = nullptr;
        }
        init();
        isReverseMode = false;
        qDebug() << "倒放停止";
    }
}

// 倒放跳转
void Controller::seekReverse(qint64 seekPos) {
    seekMutex.lock();
    isDecoderSeek = true;
    isDisplayerSeek = true;
    this->seekPos = seekPos;
    seekMutex.unlock();
    qDebug() << "倒放跳转: " << seekPos;
}

// 设置播放速率
void Controller::setPlaybackRate(float ratio, bool isReverse) {
    if (!isReverse) {
        // 正常播放速率由 MainWindow 的 QMediaPlayer 处理
        return;
    }
    if (reverseDecoder) {
        reverseDecoder->m_DifferTime = 40.0 / ratio;
        qDebug() << "倒放速率调整: " << ratio;
    }
}

// 初始化波形图
void Controller::initWaveForm(const QString &fileName) {
    deleteWaveForm();
    QString pcmPath = QDir(QApplication::applicationDirPath()).filePath("temp/" + QFileInfo(fileName).baseName() + ".pcm");
    QDir pcmDir = QFileInfo(pcmPath).dir();
    if (!pcmDir.exists() && !pcmDir.mkpath(".")) {
        qDebug() << "无法创建 PCM 目录: " << pcmDir.path();
        return;
    }
    audioGetFrame = new GetAudio(fileName, pcmPath);
    if (!audioGetFrame || audioGetFrame->total_len <= 0) {
        qDebug() << "波形图初始化失败: " << fileName;
        delete audioGetFrame;
        audioGetFrame = nullptr;
        return;
    }
    long start_data = audioGetFrame->loadAudio(0);
    qDebug() << "波形图初始化: file=" << fileName << ", start_data=" << start_data;
}

// 清除波形图
void Controller::deleteWaveForm() {
    if (audioGetFrame) {
        delete audioGetFrame;
        audioGetFrame = nullptr;
        qDebug() << "波形图数据清除";
    }
}

// 调整波形图大小
void Controller::resizeWaveForm(int height) {
    qDebug() << "波形图高度调整: " << height;
}

// 更新波形图
void Controller::updateWaveForm(qint64 progress) {
    if (audioGetFrame) {
        long start_data = audioGetFrame->loadAudio(progress / 1000);
        qDebug() << "波形图更新: start_data=" << start_data << ", progress=" << progress;
    }
}

// 设置播放状态
void Controller::setPlaying(bool playing, bool isReverse) {
    isPlaying = playing;
    isReverseMode = isReverse;
    qDebug() << "播放状态: playing=" << playing << ", isReverse=" << isReverse;
}
