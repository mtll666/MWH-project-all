#ifndef VIDEOSLIDER_H
#define VIDEOSLIDER_H

#include <QSlider>
#include <QMouseEvent>

class VideoSlider : public QSlider
{
    Q_OBJECT

public:
    explicit VideoSlider(QWidget *parent = nullptr);
    ~VideoSlider();

    void setValue(int value);
    double pointingRatio;

signals:
    void sig_valueChanged(double);
    void sig_valueChanged_v(qint64);
    void sig_moveValueChanged(double);
    void sig_mouseLeave();
    void sig_pressed();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *) override;

private:
    int m_posX; // 当前进度条的位置
    bool isSliderMoving; // 标志位, 标记是否在播放
};

#endif // VIDEOSLIDER_H
