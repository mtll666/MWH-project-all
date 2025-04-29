#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "audioimage.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QTime>
#include <QApplication>
#include <QTimer>
#include <QPainter>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDir>
#include <QAction>
#include <QShortcut>
#include <QMenu>
#include "utils.h"
#include <QVideoSink>
#include <QVideoFrame>
#include <QUrl>
#include <QFileInfo>

// 构造函数，初始化主窗口和播放器
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    setAttribute(Qt::WA_TranslucentBackground);
    mediaPlayer = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    mediaPlayer->setAudioOutput(audioOutput);
    audioOutput->setVolume(0.5);
    mediaPlayer->setPlaybackRate(1.0);
    mediaInfoLabel = ui->mediaInfoLabel;
    mediaInfoLabel->setWordWrap(true);
    subtitleLabel = new QLabel(ui->normal_widget);
    subtitleLabel->setStyleSheet("color: white; background: rgba(0, 0, 0, 150); font-size: 14px; font-family: '微软雅黑'; padding: 4px;");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setGeometry(0, ui->normal_widget->height() - 40, ui->normal_widget->width(), 40);
    subtitleLabel->hide();
    loadedVideo = false;
    isReverse = false;
    m_playerState = QMediaPlayer::StoppedState;
    ctrl = new Controller;
    playHistory = new QVector<QString>;
    playListLocal = new QVector<QString>;
    initWidgets();
    connect2Player();
    setAcceptDrops(true);
    connect(mediaPlayer, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error error, const QString &errorString) {
        showError("媒体播放错误: " + errorString);
        qDebug() << "QMediaPlayer 错误: " << error << ", 详情: " << errorString;
    });
    connect(mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &MainWindow::onMediaStatusChanged);
    qDebug() << "主窗口初始化完成";
}

// 析构函数，释放资源
MainWindow::~MainWindow()
{
    qDebug() << "主窗口开始释放资源";
    quitCurrentReversePlay();
    deleteWaveForm();
    deleteSeekFrame();
    delete playHistory;
    delete playListLocal;
    delete ctrl;
    delete ui;
    qDebug() << "主窗口资源释放完成";
}

// 初始化界面控件
void MainWindow::initWidgets()
{
    ui->normal_widget->setMouseTracking(true);
    ui->reverse_widget->setMouseTracking(true);
    setMouseTracking(true);
    ui->normal_widget->setVisible(true);
    ui->reverse_widget->setVisible(false);
    ui->pause_botton->setVisible(false);
    mediaPlayer->setVideoOutput(ui->normal_widget);
    rtText = new QAction("播放速率", this);
    rt0_5 = new QAction("0.5x", this);
    rt0_75 = new QAction("0.75x", this);
    rt1_0 = new QAction("1.0x", this);
    rt1_25 = new QAction("1.25x", this);
    rt1_5 = new QAction("1.5x", this);
    rt2_0 = new QAction("2.0x", this);
    rt3_0 = new QAction("3.0x", this);
    ui->waveformWidget->setVisible(true);
    ui->resizeWaveformButton->raise(); // 确保按钮在顶层
    qDebug() << "控件初始化完成";
}

// 显示窗口时初始化波形图
void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    initWaveformDisplay();
    qDebug() << "窗口显示，初始化波形图";
}

// 初始化波形图显示
void MainWindow::initWaveformDisplay()
{
    if (!pAudioImage) {
        pAudioImage = new AudioImage(ui->waveformWidget, ui->waveformWidget->width(), waveformHeight);
        pAudioImage->setParent(ui->waveformWidget);
        pAudioImage->setGeometry(0, 0, ui->waveformWidget->width(), waveformHeight);
        pAudioImage->setVisible(true);
        pAudioImage->raise();
        ui->resizeWaveformButton->raise();
        qDebug() << "波形图初始化: " << ui->waveformWidget->width() << "x" << waveformHeight;
    }
}

// 连接播放器信号和槽
void MainWindow::connect2Player()
{
    connect(ui->play_button, &QPushButton::clicked, this, &MainWindow::playClicked);
    connect(ui->pause_botton, &QPushButton::clicked, this, &MainWindow::pause);
    connect(ui->volume_button, &QPushButton::clicked, this, &MainWindow::changeMute);
    connect(ui->volume_slider, &QSlider::valueChanged, this, &MainWindow::changeVolume);
    connect(ui->next_video, &QPushButton::clicked, this, [this] { changeVideo(true); });
    connect(ui->previous_video, &QPushButton::clicked, this, [this] { changeVideo(false); });
    connect(ui->addFile, &QPushButton::clicked, this, &MainWindow::openFileButtonClicked);
    connect(ui->repeat, &QPushButton::clicked, this, &MainWindow::changePlayOrder);
    connect(ui->video_slider, &VideoSlider::sig_valueChanged_v, this, &MainWindow::seek);
    connect(mediaPlayer, &QMediaPlayer::positionChanged, this, &MainWindow::positionChange);
    connect(ui->forward_seconds, &QPushButton::clicked, this, [this] { skipForwardOrBackward(true); });
    connect(ui->backward_seconds, &QPushButton::clicked, this, [this] { skipForwardOrBackward(false); });
    connect(ui->playList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        QString path = item->data(Qt::UserRole).toString();
        if (QFile::exists(path)) {
            initializeVideo(path);
            normalPlay();
        } else {
            showError("文件不存在: " + path);
        }
    });
    connect(ui->close, &QPushButton::clicked, this, &MainWindow::onCloseClicked);
    connect(ui->minimize, &QPushButton::clicked, this, &MainWindow::onMinimizeClicked);
    connect(ui->maximize, &QPushButton::clicked, this, &MainWindow::onMaximizeClicked);
    connect(ui->speedComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        float ratios[] = {0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 3.0};
        changePlayingRatio(ratios[index]);
    });
    connect(ui->resizeWaveformButton, &QPushButton::clicked, this, &MainWindow::onResizeWaveformClicked);
    connect(ui->screenshotButton, &QPushButton::clicked, this, &MainWindow::onScreenshotClicked);
    connect(ui->themeComboBox, &QComboBox::currentIndexChanged, this, [this](int index) {
        ui->centralwidget->setProperty("theme", index == 0 ? "dark" : "light");
        ui->centralwidget->style()->unpolish(ui->centralwidget);
        ui->centralwidget->style()->polish(ui->centralwidget);
        showError(index == 0 ? "已切换到深色主题" : "已切换到浅色主题");
        qDebug() << "切换主题: " << (index == 0 ? "深色" : "浅色");
    });
    connect(ui->loadSubtitleButton, &QPushButton::clicked, this, &MainWindow::onLoadSubtitleClicked);
    connect(ui->playStreamButton, &QPushButton::clicked, this, &MainWindow::playStream);
    connect(ui->reversePlayButton, &QPushButton::clicked, this, &MainWindow::onReversePlayClicked);
    connect(mediaPlayer, &QMediaPlayer::positionChanged, this, &MainWindow::updateSubtitles);
    new QShortcut(QKeySequence(Qt::Key_Space), this, [this]() { playClicked(); });
    new QShortcut(QKeySequence(Qt::Key_Left), this, [this]() { skipForwardOrBackward(false); });
    new QShortcut(QKeySequence(Qt::Key_Right), this, [this]() { skipForwardOrBackward(true); });
    new QShortcut(QKeySequence(Qt::Key_Up), this, [this]() { changeVideo(false); });
    new QShortcut(QKeySequence(Qt::Key_Down), this, [this]() { changeVideo(true); });
    new QShortcut(QKeySequence(Qt::Key_S), this, [this]() { stop(); });
    new QShortcut(QKeySequence(Qt::Key_Escape), this, [this]() { onCloseClicked(); });
    new QShortcut(QKeySequence(Qt::Key_R), this, [this]() { onReversePlayClicked(); });
    qDebug() << "信号槽和快捷键连接完成";
}

// 初始化视频文件
void MainWindow::initializeVideo(QString path)
{
    qDebug() << "初始化视频: " << path;
    currentVideoPath = path;
    ctrl->setVideoPath(path);
    deleteWaveForm();
    deleteSeekFrame();
    initWaveForm(path);
    initVideoInfo(path);
    playHistory->append(path);
    subtitles.clear();
    subtitleLabel->hide();
    currentPosition = 0;
}

// 正常播放视频
void MainWindow::normalPlay()
{
    if (!loadedVideo) {
        showError("未加载视频文件");
        qDebug() << "播放失败: 未加载视频";
        return;
    }
    qDebug() << "开始正常播放: " << currentVideoPath;
    try {
        QFileInfo fileInfo(currentVideoPath);
        if (!fileInfo.exists() && !currentVideoPath.startsWith("http")) {
            showError("视频文件不存在: " + currentVideoPath);
            qDebug() << "视频文件不存在: " << currentVideoPath;
            return;
        }
        QUrl sourceUrl = currentVideoPath.startsWith("http") ? QUrl(currentVideoPath) : QUrl::fromLocalFile(fileInfo.absoluteFilePath());
        mediaPlayer->stop();
        mediaPlayer->setSource(sourceUrl);
        if (currentPosition > 0 && currentPosition <= durationMs) {
            mediaPlayer->setPosition(currentPosition);
            qDebug() << "从定位播放: " << currentPosition;
        }
        mediaPlayer->play();
        m_playerState = QMediaPlayer::PlayingState;
        ui->play_button->setVisible(false);
        ui->pause_botton->setVisible(true);
        showNormalWidget();
        ctrl->setPlaying(true, false);
        qDebug() << "正常播放启动";
    } catch (const std::exception &e) {
        showError("播放失败: " + QString(e.what()));
        qDebug() << "正常播放异常: " << e.what();
    }
}

// 停止播放
void MainWindow::stop()
{
    qDebug() << "停止播放";
    if (isReverse) {
        quitCurrentReversePlay();
    } else {
        mediaPlayer->stop();
        m_playerState = QMediaPlayer::StoppedState;
        ui->play_button->setVisible(true);
        ui->pause_botton->setVisible(false);
        subtitleLabel->hide();
        ctrl->setPlaying(false, false);
        currentPosition = 0;
    }
}

// 初始化视频信息
void MainWindow::initVideoInfo(QString path)
{
    qDebug() << "初始化视频信息: " << path;
    QUrl sourceUrl = path.startsWith("http") ? QUrl(path) : QUrl::fromLocalFile(QFileInfo(path).absoluteFilePath());
    mediaPlayer->setSource(sourceUrl);
    QTimer::singleShot(100, this, [this, path, sourceUrl]() {
        durationMs = mediaPlayer->duration();
        if (durationMs > 0) {
            durationS = durationMs / 1000;
            duration = durationMs * 1000;
            ui->video_slider->setMaximum(durationMs);
            QString totalTime = QTime(0, 0, 0).addMSecs(durationMs).toString("hh:mm:ss");
            ui->current_time->setText("00:00:00 / " + totalTime);
            loadedVideo = true;
            QString info = getVideoInfo(path);
            if (!path.startsWith("http")) {
                QFileInfo fileInfo(path);
                info += QString("文件大小: %1 MB\n").arg(fileInfo.size() / (1024.0 * 1024.0), 0, 'f', 2);
                info += QString("格式: %1\n").arg(fileInfo.suffix().toUpper());
            } else {
                info += QString("流地址: %1\n").arg(path);
            }
            mediaInfoLabel->setText(info);
            qDebug() << "视频信息初始化成功: 总时长=" << durationMs << ", 详细信息=" << info;
        } else {
            showError("无法获取视频时长: " + path);
            qDebug() << "无法获取视频时长: " << path;
            mediaPlayer->setSource(QUrl());
        }
    });
}

// 初始化波形图
void MainWindow::initWaveForm(QString fileName)
{
    QDir tempDir(QApplication::applicationDirPath() + "/temp");
    if (!tempDir.exists()) {
        if (!tempDir.mkpath(".")) {
            showError("无法创建 temp 目录");
            qDebug() << "无法创建 temp 目录: " << tempDir.path();
            return;
        }
    }
    QString pcmPath = tempDir.filePath(QFileInfo(fileName).baseName() + ".pcm");
    QFile pcmFile(pcmPath);
    if (!pcmFile.exists()) {
        qDebug() << "PCM 文件未生成: " << pcmPath;
        showError("PCM 文件未生成: " + pcmPath);
    } else if (pcmFile.size() < 1024) {
        qDebug() << "PCM 文件无效: 大小=" << pcmFile.size() << " bytes";
        showError("PCM 文件无效: " + pcmPath);
    } else {
        qDebug() << "PCM 文件检查通过: 大小=" << pcmFile.size() << " bytes";
    }
    ctrl->initWaveForm(fileName);
    if (pAudioImage) {
        pAudioImage->setVideoPath(fileName);
        pAudioImage->set_startdata(0);
        pAudioImage->update();
        pAudioImage->repaint();
        ui->waveformWidget->update();
        ui->waveformWidget->setVisible(true);
        ui->resizeWaveformButton->raise();
        qDebug() << "波形图初始化: file=" << fileName << ", pcm=" << pcmPath << ", widgetVisible=" << ui->waveformWidget->isVisible();
    }
}

// 清除波形图
void MainWindow::deleteWaveForm()
{
    ctrl->deleteWaveForm();
    if (pAudioImage) {
        pAudioImage->setVideoPath(currentVideoPath);
        pAudioImage->set_startdata(0);
        pAudioImage->update();
        pAudioImage->repaint();
        ui->waveformWidget->update();
        ui->waveformWidget->setVisible(true);
        ui->resizeWaveformButton->raise();
    }
    qDebug() << "波形图数据清除";
}

// 初始化视频帧定位
void MainWindow::initSeekFrame(QString fileName)
{
    deleteSeekFrame();
    currVideoSeekFrame = new SeekFrame(fileName, 1000, 9999);
    qDebug() << "初始化视频帧定位: " << fileName;
}

// 清除视频帧定位
void MainWindow::deleteSeekFrame()
{
    if (currVideoSeekFrame) {
        delete currVideoSeekFrame;
        currVideoSeekFrame = nullptr;
        qDebug() << "清除视频帧定位";
    }
}

// 更新播放进度
void MainWindow::positionChange(qint64 progress)
{
    if (!isReverse) {
        ui->video_slider->setValue(progress);
        currentPosition = progress;
        QString currentTimeStr = QTime(0, 0, 0).addMSecs(progress).toString("hh:mm:ss");
        QString totalTime = QTime(0, 0, 0).addMSecs(durationMs).toString("hh:mm:ss");
        ui->current_time->setText(currentTimeStr + " / " + totalTime);
        if (pAudioImage) {
            pAudioImage->set_startdata(progress / 1000);
            pAudioImage->update();
            pAudioImage->repaint();
            ui->waveformWidget->update();
            ui->waveformWidget->setVisible(true);
            ui->resizeWaveformButton->raise();
        }
        ctrl->updateWaveForm(progress);
        qDebug() << "波形图更新: start_data=" << (progress / 1000) << ", progress=" << progress << ", widgetVisible=" << ui->waveformWidget->isVisible();
    }
}

// 处理媒体状态变化
void MainWindow::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia && isRepeat && !isReverse) {
        qDebug() << "播放结束，循环播放";
        mediaPlayer->setPosition(0);
        mediaPlayer->play();
        currentPosition = 0;
    }
}

// 播放按钮点击
void MainWindow::playClicked()
{
    qDebug() << "播放按钮点击, isReverse=" << isReverse << ", playerState=" << m_playerState;
    if (isReverse) {
        reversePlay(currentVideoPath);
    } else if (!currentVideoPath.isEmpty()) {
        if (m_playerState == QMediaPlayer::PausedState) {
            play();
        } else {
            normalPlay();
        }
    } else {
        showError("播放列表为空");
        qDebug() << "播放失败: 无视频文件";
    }
}

// 暂停播放
void MainWindow::pause()
{
    qDebug() << "暂停播放, isReverse=" << isReverse;
    if (isReverse) {
        reversePause();
    } else {
        mediaPlayer->pause();
        m_playerState = QMediaPlayer::PausedState;
        ui->play_button->setVisible(true);
        ui->pause_botton->setVisible(false);
        ctrl->setPlaying(false, false);
    }
}

// 恢复播放
void MainWindow::play()
{
    qDebug() << "恢复播放, isReverse=" << isReverse << ", position=" << currentPosition;
    if (isReverse) {
        if (reverseDisplayer) {
            reverseDisplayer->resumeThread();
            m_playerState = QMediaPlayer::PlayingState;
            ctrl->setPlaying(true, true);
        }
    } else {
        mediaPlayer->play();
        m_playerState = QMediaPlayer::PlayingState;
        ui->play_button->setVisible(false);
        ui->pause_botton->setVisible(true);
        ctrl->setPlaying(true, false);
    }
}

// 跳转到指定位置
void MainWindow::seek(qint64 pos)
{
    qDebug() << "跳转到位置: " << pos;
    if (isReverse) {
        reverseSeek(pos);
    } else {
        mediaPlayer->setPosition(pos);
        currentPosition = pos;
        positionChange(pos);
    }
}

// 调整音量
void MainWindow::changeVolume(int volume)
{
    if (audioOutput) {
        audioOutput->setVolume(volume / 100.0);
        qDebug() << "音量调整: " << volume;
    }
}

// 切换静音状态
void MainWindow::changeMute()
{
    if (audioOutput) {
        m_playerMuted = !m_playerMuted;
        audioOutput->setMuted(m_playerMuted);
        qDebug() << "静音状态: " << m_playerMuted;
    }
}

// 获取当前音量
int MainWindow::volume() const
{
    return audioOutput ? audioOutput->volume() * 100 : 0;
}

// 快进或快退
void MainWindow::skipForwardOrBackward(bool forward)
{
    qDebug() << "快进/快退: " << (forward ? "向前" : "向后");
    if (isReverse) {
        reverseSkipForwardOrBackward(forward);
    } else {
        jump(forward ? 5 : -5);
    }
}

// 跳转指定秒数
void MainWindow::jump(int seconds)
{
    qint64 newPos = currentPosition + seconds * 1000;
    if (newPos < 0) newPos = 0;
    if (newPos > durationMs) newPos = durationMs;
    qDebug() << "跳转: " << seconds << "秒, 新位置=" << newPos;
    seek(newPos);
}

// 倒放模式快进或快退
void MainWindow::reverseSkipForwardOrBackward(bool forward)
{
    qint64 newPos = currentPosition + (forward ? -1000 : 1000);
    if (newPos < 0) newPos = 0;
    if (newPos > duration) newPos = duration;
    qDebug() << "倒放跳转: " << (forward ? "向前" : "向后") << ", 新位置=" << newPos;
    reverseSeek(newPos);
}

// 打开文件对话框
void MainWindow::openFileButtonClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("打开文件"), "", tr("媒体文件 (*.mp4 *.avi *.mkv *.mp3 *.wav)"));
    if (!fileName.isEmpty()) {
        qDebug() << "打开文件: " << fileName;
        addVideoItem(fileName);
        initializeVideo(fileName);
        normalPlay();
    }
}

// 添加视频到播放列表
void MainWindow::addVideoItem(QString path)
{
    if (!isValidVideoFile(path)) {
        showError("不支持的文件格式: " + path);
        qDebug() << "无效文件格式: " << path;
        return;
    }
    QString fileName = getFileName(path);
    QListWidgetItem *item = new QListWidgetItem(fileName, ui->playList);
    item->setData(Qt::UserRole, path);
    playListLocal->append(path);
    writePlayList(*playListLocal);
    qDebug() << "添加视频到播放列表: " << fileName;
}

// 切换播放视频
void MainWindow::changeVideo(bool next)
{
    if (playListLocal->isEmpty()) {
        showError("播放列表为空");
        qDebug() << "播放列表为空，无法切换";
        return;
    }
    int currentIndex = -1;
    for (int i = 0; i < playListLocal->size(); ++i) {
        if (playListLocal->at(i) == currentVideoPath) {
            currentIndex = i;
            break;
        }
    }
    int newIndex = next ? currentIndex + 1 : currentIndex - 1;
    if (newIndex < 0 || newIndex >= playListLocal->size()) {
        showError(next ? "已是最后一个视频" : "已是第一个视频");
        qDebug() << (next ? "已是最后一个视频" : "已是第一个视频");
        return;
    }
    QString newPath = playListLocal->at(newIndex);
    if (QFile::exists(newPath)) {
        qDebug() << "切换视频: " << newPath;
        initializeVideo(newPath);
        normalPlay();
        highlightInFileList();
    } else {
        showError("文件不存在: " + newPath);
        qDebug() << "文件不存在: " << newPath;
        playListLocal->removeAt(newIndex);
        writePlayList(*playListLocal);
        changeVideo(next);
    }
}

// 高亮当前播放文件
void MainWindow::highlightInFileList()
{
    for (int i = 0; i < ui->playList->count(); ++i) {
        QListWidgetItem *item = ui->playList->item(i);
        if (item->data(Qt::UserRole).toString() == currentVideoPath) {
            ui->playList->setCurrentItem(item);
            qDebug() << "高亮播放文件: " << currentVideoPath;
            break;
        }
    }
}

// 显示正常播放窗口
void MainWindow::showNormalWidget()
{
    ui->normal_widget->setVisible(true);
    ui->reverse_widget->setVisible(false);
    subtitleLabel->setParent(ui->normal_widget);
    subtitleLabel->setGeometry(0, ui->normal_widget->height() - 40, ui->normal_widget->width(), 40);
    qDebug() << "显示正常播放窗口";
}

// 显示倒放窗口
void MainWindow::showReverseWidget()
{
    ui->normal_widget->setVisible(false);
    ui->reverse_widget->setVisible(true);
    subtitleLabel->setParent(ui->reverse_widget);
    subtitleLabel->setGeometry(0, ui->reverse_widget->height() - 40, ui->reverse_widget->width(), 40);
    qDebug() << "显示倒放窗口";
}

// 切换播放顺序
void MainWindow::changePlayOrder()
{
    isRepeat = !isRepeat;
    ui->repeat->setStyleSheet(isRepeat ?
                                  "QPushButton{ image: url(:new/image/repeat.png); border-radius: 5px; }" :
                                  "QPushButton{ image: url(:new/image/inorder.png); border-radius: 5px; }");
    showError(isRepeat ? "已切换到循环播放" : "已切换到顺序播放");
    qDebug() << "切换到" << (isRepeat ? "循环播放" : "顺序播放");
}

// 倒放视频
void MainWindow::reversePlay(QString fileName)
{
    qDebug() << "开始倒放: " << fileName;
    try {
        if (!ctrl->startReversePlay(fileName, ui->reverse_widget)) {
            showError("倒放初始化失败: " + fileName);
            qDebug() << "倒放初始化失败: " << fileName;
            return;
        }
        reverseDecoder = ctrl->getReverseDecoder();
        reverseDisplayer = ctrl->getReverseDisplayer();
        if (!reverseDecoder || !reverseDisplayer) {
            showError("倒放解码器或显示器初始化失败");
            qDebug() << "倒放失败: 解码器或显示器为空";
            ctrl->stopReversePlay();
            return;
        }
        duration = reverseDecoder->duration;
        if (reverseDecoder->format_ctx && reverseDecoder->video_stream_index >= 0) {
            double rt = av_q2d(reverseDecoder->format_ctx->streams[reverseDecoder->video_stream_index]->time_base);
            reverseDurationSecond = qint64(((double)duration) * rt);
            ui->video_slider->setMaximum(duration);
        } else {
            throw std::runtime_error("无效的视频流");
        }
        mediaPlayer->stop();
        isReverse = true;
        loadedVideo = true;
        m_playerState = QMediaPlayer::PlayingState;
        ui->play_button->setVisible(false);
        ui->pause_botton->setVisible(true);
        showReverseWidget();
        lastSecond = 1e10;
        QTimer::singleShot(100, this, [this]() {
            if (reverseDisplayer) {
                connect(reverseDisplayer, &ReverseDisplay::SendOneFrame, ui->reverse_widget, &VideoFrameDisplay::slotSetOneFrame);
                connect(reverseDisplayer, &ReverseDisplay::SendTime, this, &MainWindow::reverseShowRatio);
                connect(reverseDisplayer, &ReverseDisplay::SendSecond, this, &MainWindow::recieveReverseSecond);
                qDebug() << "倒放信号连接完成";
            }
        });
        qDebug() << "倒放初始化成功";
    } catch (const std::exception &e) {
        showError("倒放失败: " + QString(e.what()));
        qDebug() << "倒放异常: " << e.what();
        ctrl->stopReversePlay();
    }
}

// 退出倒放模式
void MainWindow::quitCurrentReversePlay()
{
    qDebug() << "退出倒放模式";
    ctrl->stopReversePlay();
    reverseDecoder = nullptr;
    reverseDisplayer = nullptr;
    isReverse = false;
    showNormalWidget();
}

// 更新倒放进度
void MainWindow::reverseShowRatio(qint64 pts)
{
    ui->video_slider->setValue(int(pts));
    QString currentTimeStr = QTime(0, 0, 0).addMSecs(pts / 1000).toString("hh:mm:ss");
    QString totalTime = QTime(0, 0, 0).addMSecs(duration / 1000).toString("hh:mm:ss");
    ui->current_time->setText(currentTimeStr + " / " + totalTime);
    if (pAudioImage) {
        pAudioImage->set_startdata(pts / 1000);
        pAudioImage->update();
        pAudioImage->repaint();
        ui->waveformWidget->update();
        ui->waveformWidget->setVisible(true);
        ui->resizeWaveformButton->raise();
    }
    qDebug() << "更新倒放进度: " << pts;
}

// 倒放暂停
void MainWindow::reversePause()
{
    qDebug() << "倒放暂停, 当前状态: " << (reverseDisplayer ? (reverseDisplayer->pause_ ? "暂停" : "播放") : "无显示器");
    if (isReverse && reverseDisplayer) {
        if (!reverseDisplayer->pause_) {
            ui->play_button->setVisible(true);
            ui->pause_botton->setVisible(false);
            reverseDisplayer->pauseThread();
            m_playerState = QMediaPlayer::PausedState;
            ctrl->setPlaying(false, true);
        } else {
            ui->play_button->setVisible(false);
            ui->pause_botton->setVisible(true);
            reverseDisplayer->resumeThread();
            m_playerState = QMediaPlayer::PlayingState;
            ctrl->setPlaying(true, true);
        }
    }
}

// 倒放跳转
void MainWindow::reverseSeek(qint64 seekPos)
{
    qDebug() << "倒放跳转到: " << seekPos;
    if (isReverse) {
        lastSecond = reverseDurationSecond;
        ctrl->seekReverse(seekPos);
    }
}

// 接收倒放时间
void MainWindow::recieveReverseSecond(double secondReceive)
{
    if (secondReceive < 0.3) {
        if (isRepeat) {
            reverseSeek(qint64(ui->video_slider->maximum()));
        } else {
            reversePause();
        }
    }
    qint64 second = qint64(secondReceive);
    if (second - lastSecond > 1) {
        lastSecond = second;
    }
    if (second < lastSecond) {
        lastSecond = second;
        emit sig_reverseProgress(second);
        ctrl->updateWaveForm(second * 1000);
        if (pAudioImage) {
            pAudioImage->set_startdata(second);
            pAudioImage->update();
            pAudioImage->repaint();
            ui->waveformWidget->update();
            ui->waveformWidget->setVisible(true);
            ui->resizeWaveformButton->raise();
        }
    }
    qDebug() << "倒放时间更新: " << secondReceive;
}

// 播放流媒体
void MainWindow::playStream()
{
    QString url = ui->streamUrlInput->text().trimmed();
    qDebug() << "尝试播放流: " << url;
    if (url.isEmpty()) {
        showError("请输入流媒体 URL");
        qDebug() << "流播放失败: URL 为空";
        return;
    }
    if (!isValidStreamUrl(url)) {
        showError("无效的流媒体 URL: " + url);
        qDebug() << "流播放失败: 无效 URL";
        return;
    }
    try {
        mediaPlayer->stop();
        QVariantMap options;
        options["extra_hw_frames"] = 64;
        mediaPlayer->setSource(QUrl(url));
        mediaPlayer->setProperty("options", options);
        if (currentPosition > 0) {
            mediaPlayer->setPosition(currentPosition);
            qDebug() << "流媒体从定位播放: " << currentPosition;
        }
        mediaPlayer->play();
        m_playerState = QMediaPlayer::PlayingState;
        ui->play_button->setVisible(false);
        ui->pause_botton->setVisible(true);
        loadedVideo = true;
        showNormalWidget();
        currentVideoPath = url;
        ctrl->setVideoPath(url);
        initWaveForm(url);
        QTimer::singleShot(100, this, [this, url]() {
            durationMs = mediaPlayer->duration();
            if (durationMs > 0) {
                durationS = durationMs / 1000;
                duration = durationMs * 1000;
                ui->video_slider->setMaximum(durationMs);
                QString totalTime = QTime(0, 0, 0).addMSecs(durationMs).toString("hh:mm:ss");
                ui->current_time->setText("00:00:00 / " + totalTime);
                QString info = getVideoInfo(url);
                info += QString("流地址: %1\n").arg(url);
                mediaInfoLabel->setText(info);
                qDebug() << "流播放初始化成功: " << url << ", 时长=" << durationMs;
            } else {
                qDebug() << "无法获取流时长: " << url;
            }
        });
        showError("流媒体播放成功: " + url);
    } catch (const std::exception &e) {
        showError("流媒体播放失败: " + QString(e.what()));
        qDebug() << "流播放异常: " << e.what();
    }
}

// 倒放按钮点击
void MainWindow::onReversePlayClicked()
{
    qDebug() << "倒放按钮点击";
    if (!loadedVideo || currentVideoPath.isEmpty()) {
        showError("请先加载视频文件");
        qDebug() << "倒放失败: 未加载视频";
        return;
    }
    if (isReverse) {
        quitCurrentReversePlay();
        normalPlay();
    } else {
        reversePlay(currentVideoPath);
    }
}

// 显示错误信息
void MainWindow::showError(const QString &message)
{
    ui->errorLabel->setText(message);
    QTimer::singleShot(3000, this, [this]() { ui->errorLabel->setText(""); });
    qDebug() << "错误信息: " << message;
}

// 加载字幕
void MainWindow::onLoadSubtitleClicked()
{
    QString srtFile = QFileDialog::getOpenFileName(this, tr("加载字幕"), "", tr("字幕文件 (*.srt)"));
    if (srtFile.isEmpty()) return;
    subtitles = loadSubtitles(srtFile);
    if (subtitles.isEmpty()) {
        showError("无法加载字幕: " + srtFile);
        qDebug() << "字幕加载失败: " << srtFile;
    } else {
        showError("字幕加载成功: " + srtFile);
        subtitleLabel->show();
        qDebug() << "字幕加载成功: " << srtFile;
    }
}

// 更新字幕显示
void MainWindow::updateSubtitles(qint64 position)
{
    if (subtitles.isEmpty()) return;
    QString subtitleText;
    for (const Subtitle &subtitle : subtitles) {
        if (position >= subtitle.startTime && position <= subtitle.endTime) {
            subtitleText = subtitle.text;
            break;
        }
    }
    subtitleLabel->setText(subtitleText);
    subtitleLabel->setVisible(!subtitleText.isEmpty());
    qDebug() << "更新字幕: position=" << position << ", text=" << subtitleText;
}

// 截图
void MainWindow::onScreenshotClicked()
{
    if (!loadedVideo) {
        showError("未加载视频，无法截图");
        qDebug() << "截图失败: 未加载视频";
        return;
    }
    QImage screenshot;
    if (isReverse) {
        screenshot = ui->reverse_widget->GetImage();
    } else {
        QVideoFrame frame = mediaPlayer->videoSink()->videoFrame();
        if (frame.isValid()) {
            screenshot = frame.toImage();
        }
    }
    if (screenshot.isNull()) {
        showError("无法捕获截图");
        qDebug() << "截图失败: " << (isReverse ? "倒放模式图像为空" : "正常模式图像为空");
        return;
    }
    QString savePath = QDir::homePath() + "/截图_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".png";
    if (screenshot.save(savePath, "PNG")) {
        showError("截图已保存至: " + savePath);
        qDebug() << "截图保存成功: " << savePath;
    } else {
        showError("保存截图失败");
        qDebug() << "截图保存失败: " << savePath;
    }
}

// 调整波形图大小
void MainWindow::onResizeWaveformClicked()
{
    waveformHeight = (waveformHeight == 50) ? 100 : 50;
    if (pAudioImage) {
        pAudioImage->setSize(ui->waveformWidget->width(), waveformHeight);
        pAudioImage->update();
        pAudioImage->repaint();
        ui->waveformWidget->update();
        ui->waveformWidget->setVisible(true);
        ui->resizeWaveformButton->raise();
    }
    ui->waveformWidget->setMinimumHeight(waveformHeight);
    ui->waveformWidget->setMaximumHeight(waveformHeight);
    ctrl->resizeWaveForm(waveformHeight);
    showError(QString("波形图高度已更改为 %1 像素").arg(waveformHeight));
    qDebug() << "波形图高度调整: " << waveformHeight;
}

// 排序播放列表
void MainWindow::sortPlayList()
{
    if (playListLocal->isEmpty()) {
        showError("播放列表为空，无法排序");
        qDebug() << "排序失败: 播放列表为空";
        return;
    }
    std::sort(playListLocal->begin(), playListLocal->end(), [](const QString &a, const QString &b) {
        return QFileInfo(a).fileName().toLower() < QFileInfo(b).fileName().toLower();
    });
    ui->playList->clear();
    for (const QString &path : *playListLocal) {
        QString fileName = getFileName(path);
        QListWidgetItem *item = new QListWidgetItem(fileName, ui->playList);
        item->setData(Qt::UserRole, path);
    }
    writePlayList(*playListLocal);
    showError("播放列表已排序");
    qDebug() << "播放列表排序完成";
}

// 显示播放历史
void MainWindow::showHistory()
{
    if (playHistory->isEmpty()) {
        showError("播放历史为空");
        qDebug() << "播放历史为空";
        return;
    }
    QString history;
    for (const QString &path : *playHistory) {
        history += getFileName(path) + "\n";
    }
    QMessageBox::information(this, "播放历史", history, QMessageBox::Ok);
    qDebug() << "显示播放历史";
}

// 关闭窗口
void MainWindow::onCloseClicked()
{
    qDebug() << "关闭窗口";
    QApplication::quit();
}

// 最小化窗口
void MainWindow::onMinimizeClicked()
{
    qDebug() << "最小化窗口";
    showMinimized();
}

// 最大化/还原窗口
void MainWindow::onMaximizeClicked()
{
    qDebug() << "最大化窗口";
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
}

// 更改播放速率
void MainWindow::changePlayingRatio(float ratio)
{
    qDebug() << "更改播放速率: " << ratio;
    if (isReverse) {
        ctrl->setPlaybackRate(ratio, true);
    } else {
        mediaPlayer->setPlaybackRate(ratio);
    }
    showError(QString("播放速率已更改为 %1x").arg(ratio));
}

// 窗口大小调整事件
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    ctrl->resizeWaveForm(waveformHeight);
    if (subtitleLabel) {
        QWidget *parentWidget = isReverse ? static_cast<QWidget*>(ui->reverse_widget) : static_cast<QWidget*>(ui->normal_widget);
        subtitleLabel->setGeometry(0, parentWidget->height() - 40, parentWidget->width(), 40);
    }
    if (pAudioImage) {
        pAudioImage->update();
        pAudioImage->repaint();
        ui->waveformWidget->update();
        ui->waveformWidget->setVisible(true);
        ui->resizeWaveformButton->raise();
    }
}

// 鼠标按下事件
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    static QTime lastPress = QTime::currentTime();
    QTime currentTime = QTime::currentTime();
    if (lastPress.msecsTo(currentTime) < 200) return;
    lastPress = currentTime;
    if (event && event->button() == Qt::LeftButton) {
        qDebug() << "鼠标按下";
        m_drag = true;
        dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
        resizeRegion = getResizeRegion(event->pos());
        mouseDownRect = geometry();
        resizeDownPos = event->globalPosition().toPoint();
    }
    QMainWindow::mousePressEvent(event);
}

// 鼠标移动事件
void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (event && m_drag && (event->buttons() & Qt::LeftButton)) {
        if (resizeRegion == Default) {
            move(event->globalPosition().toPoint() - dragPos);
        }
    } else if (event) {
        setResizeCursor(getResizeRegion(event->pos()));
    }
    QMainWindow::mouseMoveEvent(event);
}

// 鼠标释放事件
void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    m_drag = false;
    setCursor(Qt::ArrowCursor);
    QMainWindow::mouseReleaseEvent(event);
}

// 获取调整大小区域
MainWindow::ResizeRegion MainWindow::getResizeRegion(QPoint clientPos)
{
    int x = clientPos.x();
    int y = clientPos.y();
    int w = width();
    int h = height();
    int border = resizeBorderWidth;
    if (x < border && y < border) return NorthWest;
    if (x > w - border && y < border) return NorthEast;
    if (x < border && y > h - border) return SouthWest;
    if (x > w - border && y > h - border) return SouthEast;
    if (x < border) return West;
    if (x > w - border) return East;
    if (y < border) return North;
    if (y > h - border) return South;
    return Default;
}

// 设置调整大小光标
void MainWindow::setResizeCursor(ResizeRegion region)
{
    switch (region) {
    case NorthWest:
    case SouthEast:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case NorthEast:
    case SouthWest:
        setCursor(Qt::SizeBDiagCursor);
        break;
    case North:
    case South:
        setCursor(Qt::SizeVerCursor);
        break;
    case West:
    case East:
        setCursor(Qt::SizeHorCursor);
        break;
    default:
        setCursor(Qt::ArrowCursor);
        break;
    }
}

// 绘制窗口
void MainWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen(Qt::gray, 2);
    painter.setPen(pen);
    painter.setBrush(QColor(40, 45, 48, 200));
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 15, 15);
    QMainWindow::paintEvent(event);
}

// 右键菜单事件
void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    if (!event) return;
    QMenu menu(this);
    menu.addAction(rtText);
    menu.addAction(rt0_5);
    menu.addAction(rt0_75);
    menu.addAction(rt1_0);
    menu.addAction(rt1_25);
    menu.addAction(rt1_5);
    menu.addAction(rt2_0);
    menu.addAction(rt3_0);
    QAction *sortAction = new QAction("排序播放列表", this);
    QAction *historyAction = new QAction("查看播放历史", this);
    menu.addAction(sortAction);
    menu.addAction(historyAction);
    connect(rt0_5, &QAction::triggered, this, [this] { changePlayingRatio(0.5); });
    connect(rt0_75, &QAction::triggered, this, [this] { changePlayingRatio(0.75); });
    connect(rt1_0, &QAction::triggered, this, [this] { changePlayingRatio(1.0); });
    connect(rt1_25, &QAction::triggered, this, [this] { changePlayingRatio(1.25); });
    connect(rt1_5, &QAction::triggered, this, [this] { changePlayingRatio(1.5); });
    connect(rt2_0, &QAction::triggered, this, [this] { changePlayingRatio(2.0); });
    connect(rt3_0, &QAction::triggered, this, [this] { changePlayingRatio(3.0); });
    connect(sortAction, &QAction::triggered, this, &MainWindow::sortPlayList);
    connect(historyAction, &QAction::triggered, this, &MainWindow::showHistory);
    menu.exec(event->globalPos());
}

// 拖放事件
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

// 处理拖放文件
void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();
        for (const QUrl &url : urlList) {
            QString filePath = url.toLocalFile();
            if (isValidVideoFile(filePath)) {
                qDebug() << "拖放文件: " << filePath;
                addVideoItem(filePath);
                initializeVideo(filePath);
                normalPlay();
                break;
            }
        }
    }
}
