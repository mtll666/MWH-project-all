#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QLabel>
#include <QVector>
#include "controller.h"
#include "reversedecode.h"
#include "reversedisplay.h"
#include "getaudio.h"
#include "seekframe.h"
#include "utils.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class AudioImage;

// 主窗口类，管理媒体播放和用户交互
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void sig_reverseProgress(qint64); // 倒放进度信号

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void playClicked();
    void pause();
    void play();
    void seek(qint64 pos);
    void changeVolume(int volume);
    void changeMute();
    void skipForwardOrBackward(bool forward);
    void openFileButtonClicked();
    void changeVideo(bool next);
    void changePlayOrder();
    void positionChange(qint64 progress);
    void reverseShowRatio(qint64 pts);
    void recieveReverseSecond(double second);
    void onCloseClicked();
    void onMinimizeClicked();
    void onMaximizeClicked();
    void onResizeWaveformClicked();
    void onScreenshotClicked();
    void onLoadSubtitleClicked();
    void updateSubtitles(qint64 position);
    void playStream();
    void onReversePlayClicked();
    void sortPlayList();
    void showHistory();
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);

private:
    enum ResizeRegion { Default, North, South, West, East, NorthWest, NorthEast, SouthWest, SouthEast };
    Ui::MainWindow *ui;
    QMediaPlayer *mediaPlayer = nullptr;
    QAudioOutput *audioOutput = nullptr;
    QLabel *mediaInfoLabel = nullptr;
    QLabel *subtitleLabel = nullptr;
    Controller *ctrl = nullptr;
    ReverseDecode *reverseDecoder = nullptr;
    ReverseDisplay *reverseDisplayer = nullptr;
    GetAudio *currAudioGetFrame = nullptr;
    AudioImage *pAudioImage = nullptr;
    SeekFrame *currVideoSeekFrame = nullptr;
    QVector<QString> *playHistory = nullptr;
    QVector<QString> *playListLocal = nullptr;
    QVector<Subtitle> subtitles;
    QString currentVideoPath;
    QAction *rtText = nullptr;
    QAction *rt0_5 = nullptr, *rt0_75 = nullptr, *rt1_0 = nullptr;
    QAction *rt1_25 = nullptr, *rt1_5 = nullptr, *rt2_0 = nullptr, *rt3_0 = nullptr;
    qint64 durationMs = 0, durationS = 0, duration = 0;
    qint64 currentPosition = 0;
    qint64 reverseDurationSecond = 0;
    double lastSecond = 0;
    int waveformHeight = 50;
    int resizeBorderWidth = 5;
    bool loadedVideo = false;
    bool isReverse = false;
    bool m_playerMuted = false;
    QMediaPlayer::PlaybackState m_playerState = QMediaPlayer::StoppedState;
    bool isRepeat = false;
    bool m_drag = false;
    QPoint dragPos;
    QRect mouseDownRect;
    QPoint resizeDownPos;
    ResizeRegion resizeRegion = Default;

    void initWidgets();
    void connect2Player();
    void initializeVideo(QString path);
    void normalPlay();
    void stop();
    void initVideoInfo(QString path);
    void initWaveForm(QString fileName);
    void deleteWaveForm();
    void initSeekFrame(QString fileName);
    void deleteSeekFrame();
    void showNormalWidget();
    void showReverseWidget();
    void reversePlay(QString fileName);
    void quitCurrentReversePlay();
    void reversePause();
    void reverseSeek(qint64 seekPos);
    void addVideoItem(QString path);
    void highlightInFileList();
    int volume() const;
    void jump(int seconds);
    void reverseSkipForwardOrBackward(bool forward);
    void changePlayingRatio(float ratio);
    ResizeRegion getResizeRegion(QPoint clientPos);
    void setResizeCursor(ResizeRegion region);
    void showError(const QString &message);
    void initWaveformDisplay();
};

#endif // MAINWINDOW_H
