#ifndef VIDEOFRAMEDISPLAY_H
#define VIDEOFRAMEDISPLAY_H

#include <QWidget>
#include <QImage>
#include <QPaintEvent>

class VideoFrameDisplay : public QWidget
{
    Q_OBJECT
public:
    explicit VideoFrameDisplay(QWidget *parent = nullptr);
    ~VideoFrameDisplay();

    // 获取当前帧图像
    QImage GetImage() const;

public slots:
    // 设置帧图像
    void slotSetOneFrame(const QImage &image);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QImage currentFrame; // 当前帧图像
};

#endif // VIDEOFRAMEDISPLAY_H
