#ifndef AUDIOIMAGE_H
#define AUDIOIMAGE_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>

class AudioImage : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit AudioImage(QWidget *parent = nullptr, int w = 0, int h = 0);
    ~AudioImage();

    void set_startdata(long start_data);
    void setSize(int w, int h);
    void setVideoPath(const QString &path);
    bool isValid() const;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void fallbackRender(); // 声明备用渲染函数

private:
    QString currentVideoPath;
    long start_data = 0;
    int width;
    int height;
    bool valid = false;
};

#endif // AUDIOIMAGE_H
