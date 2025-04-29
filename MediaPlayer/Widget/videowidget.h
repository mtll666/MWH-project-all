#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QVideoWidget>

class VideoWidget : public QVideoWidget
{
    Q_OBJECT

public:
    explicit VideoWidget(QWidget *parent = nullptr);

protected:
    // 按键 event, 比如退出全屏
    void keyPressEvent(QKeyEvent *event) override;
    // 实现双击全屏
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    // 单击
    void mousePressEvent(QMouseEvent *event) override;
};

#endif // VIDEOWIDGET_H
