#ifndef REVERSEDECODE_H
#define REVERSEDECODE_H

#include <QThread>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

// 前向声明
class Controller;

// 倒放解码器类，处理视频帧解码
class ReverseDecode : public QThread
{
    Q_OBJECT
public:
    explicit ReverseDecode(Controller *ctrl);
    ~ReverseDecode();
    int loadFile(QString filename);
    void quit();
    qint64 duration = 0;
    double m_DifferTime = 40.0; // 倒放时间差，单位毫秒
    AVFormatContext *format_ctx = nullptr;
    int video_stream_index = -1;

protected:
    void run() override;

private:
    Controller *ctrl;
    AVCodecContext *video_dec_ctx = nullptr;
    AVFrame *pFrame = nullptr;
    AVFrame *RGB24_pFrame = nullptr;
    SwsContext *img_convert_ctx = nullptr;
    uint8_t *out_buffer_rgb = nullptr;
    int video_width = 0;
    int video_height = 0;
};

#endif // REVERSEDECODE_H
