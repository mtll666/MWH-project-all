// 自定义视频播放窗口，支持全屏切换和鼠标交互
#include "Widget/videowidget.h"

#include <QKeyEvent>
#include <QDebug>
#include <QMouseEvent>

// 构造函数，初始化视频窗口
VideoWidget::VideoWidget(QWidget *parent)
    : QVideoWidget(parent)
{
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    // 设置黑色背景
    QPalette p = palette();
    p.setColor(QPalette::Window, Qt::black);
    setPalette(p);

    // 启用不透明绘制
    setAttribute(Qt::WA_OpaquePaintEvent);
}

// 处理键盘事件，支持 Esc 退出全屏和 Alt+Enter 切换全屏
void VideoWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape && isFullScreen()) {
        setFullScreen(false);
        event->accept();
    } else if (event->key() == Qt::Key_Enter && event->modifiers() & Qt::Key_Alt) {
        setFullScreen(!isFullScreen());
        event->accept();
    } else {
        QVideoWidget::keyPressEvent(event);
    }
}

// 鼠标双击切换全屏
void VideoWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    setFullScreen(!isFullScreen());
    event->accept();
}

// 鼠标按下事件
void VideoWidget::mousePressEvent(QMouseEvent *event)
{
    qDebug() << "鼠标按下";
    QVideoWidget::mousePressEvent(event);
}
