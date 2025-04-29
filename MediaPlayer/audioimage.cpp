#include "audioimage.h"
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QOpenGLShaderProgram>
#include <QApplication>
#include <QDir>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

// 构造函数，初始化波形图
AudioImage::AudioImage(QWidget *parent, int w, int h)
    : QOpenGLWidget(parent), width(w), height(h)
{
    setMinimumSize(w, h);
    setMaximumSize(w, h);
    setAutoFillBackground(true);
    setVisible(true);
    setAttribute(Qt::WA_AlwaysStackOnTop); // 确保控件在顶层
    qDebug() << "构造: w=" << w << ", h=" << h;
}

// 析构函数
AudioImage::~AudioImage()
{
    makeCurrent();
    doneCurrent();
    qDebug() << "音频波形图析构";
}

// 设置波形起始数据
void AudioImage::set_startdata(long start_data)
{
    this->start_data = start_data;
    update();
    qDebug() << "设置波形起始数据: " << start_data;
}

// 设置波形图尺寸
void AudioImage::setSize(int w, int h)
{
    width = w;
    height = h;
    setMinimumSize(w, h);
    setMaximumSize(w, h);
    update();
    qDebug() << "调整波形图尺寸: " << w << "x" << h;
}

// 设置视频路径
void AudioImage::setVideoPath(const QString &path)
{
    currentVideoPath = path;
    update();
    qDebug() << "设置视频路径: " << path;
}

// 检查波形图有效性
bool AudioImage::isValid() const
{
    return valid && context() && context()->isValid();
}

// 初始化 OpenGL
void AudioImage::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    valid = true;
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        qDebug() << "OpenGL 初始化错误: " << err;
        valid = false;
    }
    qDebug() << "OpenGL 初始化成功";
}

// 调整 OpenGL 视口
void AudioImage::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    width = w;
    height = h;
    qDebug() << "OpenGL 视口调整: " << w << "x" << h;
}

// 绘制波形图
void AudioImage::paintGL()
{
    if (!isVisible()) {
        qDebug() << "渲染失败: 控件不可见";
        fallbackRender();
        return;
    }
    if (!isValid()) {
        qDebug() << "渲染失败: OpenGL 控件无效";
        fallbackRender();
        return;
    }
    glClear(GL_COLOR_BUFFER_BIT);
    if (currentVideoPath.isEmpty()) {
        qDebug() << "渲染失败: 视频路径为空";
        return;
    }
    QString pcmPath = QDir(QApplication::applicationDirPath()).filePath("temp/" + QFileInfo(currentVideoPath).baseName() + ".pcm");
    QFile pcmFile(pcmPath);
    if (!pcmFile.exists()) {
        qDebug() << "渲染失败: PCM 文件不存在: " << pcmPath;
        return;
    }
    if (!pcmFile.open(QIODevice::ReadOnly)) {
        qDebug() << "渲染失败: 无法打开 PCM 文件: " << pcmPath;
        return;
    }
    qint64 fileSize = pcmFile.size();
    if (fileSize < 1024) {
        qDebug() << "渲染失败: PCM 文件过小: " << fileSize << " bytes";
        pcmFile.close();
        return;
    }
    // 44.1kHz双声道16位
    qint64 offset = start_data * 44100 * 4;
    if (offset >= fileSize) {
        qDebug() << "渲染失败: 偏移超出文件大小: offset=" << offset << ", size=" << fileSize;
        pcmFile.close();
        return;
    }
    if (!pcmFile.seek(offset)) {
        qDebug() << "渲染失败: 无法定位 PCM 文件: offset=" << offset;
        pcmFile.close();
        return;
    }
    QByteArray data = pcmFile.read(width * 4);
    pcmFile.close();
    if (data.isEmpty()) {
        qDebug() << "渲染失败: PCM 数据为空: offset=" << offset;
        return;
    }
    glLineWidth(2.0f);
    glBegin(GL_LINE_STRIP);
    glColor3f(0.0f, 1.0f, 0.0f);
    for (int i = 0; i < data.size() / 4; ++i) {
        int16_t sample = ((int16_t*)data.constData())[i * 2];
        float x = (float)i / width * 2.0f - 1.0f;
        float y = sample / 32768.0f * 0.8f * (height / 100.0f);
        glVertex2f(x, y);
    }
    glEnd();
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        qDebug() << "OpenGL 渲染错误: " << err;
        fallbackRender();
        return;
    }
    qDebug() << "波形图渲染: offset=" << offset << ", samples=" << data.size() / 4 << ", path=" << pcmPath;
}

// 备用渲染方法
void AudioImage::fallbackRender()
{
    if (!isVisible()) {
        qDebug() << "备用渲染失败: 控件不可见";
        return;
    }
    QImage image(width, height, QImage::Format_RGB32);
    image.fill(Qt::black);
    QPainter painter(&image);
    QPen pen(Qt::green, 2);
    painter.setPen(pen);
    QString pcmPath = QDir(QApplication::applicationDirPath()).filePath("temp/" + QFileInfo(currentVideoPath).baseName() + ".pcm");
    QFile pcmFile(pcmPath);
    if (!pcmFile.exists()) {
        qDebug() << "备用渲染失败: PCM 文件不存在: " << pcmPath;
        painter.end();
        QPainter widgetPainter(this);
        widgetPainter.drawImage(0, 0, image);
        return;
    }
    if (!pcmFile.open(QIODevice::ReadOnly)) {
        qDebug() << "备用渲染失败: 无法打开 PCM 文件: " << pcmPath;
        painter.end();
        QPainter widgetPainter(this);
        widgetPainter.drawImage(0, 0, image);
        return;
    }
    qint64 fileSize = pcmFile.size();
    if (fileSize < 1024) {
        qDebug() << "备用渲染失败: PCM 文件过小: " << fileSize << " bytes";
        pcmFile.close();
        painter.end();
        QPainter widgetPainter(this);
        widgetPainter.drawImage(0, 0, image);
        return;
    }
    qint64 offset = start_data * 44100 * 4;
    if (offset >= fileSize) {
        qDebug() << "备用渲染失败: 偏移超出文件大小: offset=" << offset << ", size=" << fileSize;
        pcmFile.close();
        painter.end();
        QPainter widgetPainter(this);
        widgetPainter.drawImage(0, 0, image);
        return;
    }
    if (pcmFile.seek(offset)) {
        QByteArray data = pcmFile.read(width * 4);
        if (!data.isEmpty()) {
            QPainterPath path;
            for (int i = 0; i < data.size() / 4; ++i) {
                int16_t sample = ((int16_t*)data.constData())[i * 2];
                float x = (float)i / width * width;
                float y = height / 2.0f - (sample / 32768.0f * height * 0.4f);
                if (i == 0) {
                    path.moveTo(x, y);
                } else {
                    path.lineTo(x, y);
                }
            }
            painter.drawPath(path);
        } else {
            qDebug() << "备用渲染失败: PCM 数据为空: offset=" << offset;
        }
    } else {
        qDebug() << "备用渲染失败: 无法定位 PCM 文件: offset=" << offset;
    }
    pcmFile.close();
    painter.end();
    QPainter widgetPainter(this);
    widgetPainter.drawImage(0, 0, image);
    qDebug() << "备用渲染完成: path=" << pcmPath;
}
