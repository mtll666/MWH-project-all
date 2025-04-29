// 实现视频帧显示控件
#include "VideoFrameDisplay.h"
#include <QPainter>
#include <QDebug>

VideoFrameDisplay::VideoFrameDisplay(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background: black; border-radius: 5px;");
}

VideoFrameDisplay::~VideoFrameDisplay()
{
}

void VideoFrameDisplay::slotSetOneFrame(const QImage &image)
{
    currentFrame = image;
    update();
    qDebug() << "更新倒放帧";
}

QImage VideoFrameDisplay::GetImage() const
{
    return currentFrame;
}

void VideoFrameDisplay::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    if (!currentFrame.isNull()) {
        QRect rect = event->rect();
        QImage scaledImage = currentFrame.scaled(rect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        int x = (rect.width() - scaledImage.width()) / 2;
        int y = (rect.height() - scaledImage.height()) / 2;
        painter.drawImage(x, y, scaledImage);
    } else {
        painter.fillRect(event->rect(), Qt::black);
    }
}
