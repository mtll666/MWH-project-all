// 自定义视频进度条控件，支持鼠标交互
#include "VideoSlider.h"
#include <QDebug>
#include <QStyle>

// 构造函数，初始化进度条
VideoSlider::VideoSlider(QWidget *parent) :
    QSlider(parent)
{
    setMouseTracking(true);
    setOrientation(Qt::Horizontal);
    isSliderMoving = false;
}

// 析构函数
VideoSlider::~VideoSlider()
{
}

// 设置进度值
void VideoSlider::setValue(int value)
{
    QSlider::setValue(value);
}

// 鼠标按下，计算进度并发射信号
void VideoSlider::mousePressEvent(QMouseEvent *event)
{
    m_posX = event->pos().x();
    double curr_x = double(m_posX);
    double curr_total = double(width());
    double ratio = curr_x / curr_total;
    int value = QStyle::sliderValueFromPosition(minimum(), maximum(), event->pos().x(), width());
    setValue(value);
    emit sig_valueChanged(ratio);
    emit sig_valueChanged_v((qint64)value);
    emit sig_pressed();
}

// 鼠标移动，发射移动比例信号
void VideoSlider::mouseMoveEvent(QMouseEvent *event)
{
    m_posX = event->pos().x();
    double curr_x = double(m_posX);
    double curr_total = double(width());
    double ratio = curr_x / curr_total;
    pointingRatio = ratio;
    emit sig_moveValueChanged(ratio);
    QSlider::mouseMoveEvent(event);
}

// 鼠标离开，发射离开信号
void VideoSlider::leaveEvent(QEvent *)
{
    emit sig_mouseLeave();
}
