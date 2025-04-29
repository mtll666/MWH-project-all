#include "utils.h"
#include <QFile>
#include <QDir>
#include <QApplication>
#include <QDebug>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

// 读取播放列表
QVector<QString>* readPlayList()
{
    QVector<QString>* playList = new QVector<QString>;
    QFile file(QDir(QApplication::applicationDirPath()).filePath("playList.dat"));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty() && QFile::exists(line)) {
                playList->append(line);
            }
        }
        file.close();
    }
    return playList;
}

// 写入播放列表
void writePlayList(const QVector<QString>& playList)
{
    QFile file(QDir(QApplication::applicationDirPath()).filePath("playList.dat"));
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const QString& path : playList) {
            out << path << "\n";
        }
        file.close();
    }
}

// 获取文件名
QString getFileName(const QString& path)
{
    return QFileInfo(path).fileName();
}

// 获取视频信息（时长、分辨率、帧率、比特率、编码器等）
QString getVideoInfo(const QString& path)
{
    AVFormatContext* fmtCtx = nullptr;
    QString info;
    if (avformat_open_input(&fmtCtx, path.toStdString().c_str(), nullptr, nullptr) >= 0) {
        avformat_find_stream_info(fmtCtx, nullptr);
        int64_t duration = fmtCtx->duration / AV_TIME_BASE;
        QString durationStr = QTime(0, 0, 0).addSecs(duration).toString("hh:mm:ss");
        info += QString("时长: %1\n").arg(durationStr);
        info += QString("比特率: %1 kbps\n").arg(fmtCtx->bit_rate / 1000);
        for (unsigned int i = 0; i < fmtCtx->nb_streams; ++i) {
            AVStream* stream = fmtCtx->streams[i];
            if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
                info += QString("视频: %1x%2, %3 fps, 编码: %4\n")
                            .arg(stream->codecpar->width)
                            .arg(stream->codecpar->height)
                            .arg(av_q2d(stream->r_frame_rate))
                            .arg(codec ? codec->name : "未知");
            } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
                info += QString("音频: %1 Hz, %2 声道, 编码: %3\n")
                            .arg(stream->codecpar->sample_rate)
                            .arg(stream->codecpar->ch_layout.nb_channels)
                            .arg(codec ? codec->name : "未知");
            }
        }
        avformat_close_input(&fmtCtx);
    } else {
        info = "无法获取媒体信息";
        qDebug() << "获取视频信息失败: " << path;
    }
    return info;
}

// 验证视频文件格式
bool isValidVideoFile(const QString& path)
{
    QString ext = QFileInfo(path).suffix().toLower();
    QStringList validExts = {"mp4", "mkv", "avi", "mp3", "wav"};
    return validExts.contains(ext);
}

// 验证流媒体 URL
bool isValidStreamUrl(const QString& url)
{
    qDebug() << "验证流 URL: " << url;
    AVFormatContext* fmtCtx = avformat_alloc_context();
    if (!fmtCtx) {
        qDebug() << "无法分配格式上下文";
        return false;
    }
    AVDictionary* options = nullptr;
    av_dict_set(&options, "rtsp_transport", "tcp", 0);
    av_dict_set(&options, "timeout", "5000000", 0); // 5秒超时
    int ret = avformat_open_input(&fmtCtx, url.toStdString().c_str(), nullptr, &options);
    av_dict_free(&options);
    if (ret != 0) {
        qDebug() << "无法打开流: " << url << ", 错误码=" << ret;
        avformat_free_context(fmtCtx);
        return false;
    }
    ret = avformat_find_stream_info(fmtCtx, nullptr);
    if (ret < 0) {
        qDebug() << "无法获取流信息: " << url << ", 错误码=" << ret;
        avformat_close_input(&fmtCtx);
        avformat_free_context(fmtCtx);
        return false;
    }
    avformat_close_input(&fmtCtx);
    avformat_free_context(fmtCtx);
    qDebug() << "流 URL 验证成功: " << url;
    return true;
}

// 加载 SRT 字幕文件
QVector<Subtitle> loadSubtitles(const QString& srtFile)
{
    QVector<Subtitle> subtitles;
    QFile file(srtFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "无法打开字幕文件: " << srtFile;
        return subtitles;
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        bool isNumber;
        line.toInt(&isNumber);
        if (isNumber) continue;

        if (line.contains("-->")) {
            Subtitle subtitle;
            QStringList timeParts = line.split(" --> ");
            if (timeParts.size() == 2) {
                QString startTimeStr = timeParts[0].replace(",", ".");
                QTime startTime = QTime::fromString(startTimeStr, "hh:mm:ss.zzz");
                subtitle.startTime = startTime.msecsSinceStartOfDay();

                QString endTimeStr = timeParts[1].replace(",", ".");
                QTime endTime = QTime::fromString(endTimeStr, "hh:mm:ss.zzz");
                subtitle.endTime = endTime.msecsSinceStartOfDay();

                QString subtitleText;
                while (!in.atEnd()) {
                    QString nextLine = in.readLine().trimmed();
                    if (nextLine.isEmpty()) break;
                    subtitleText += nextLine + "\n";
                }
                subtitle.text = subtitleText.trimmed();
                subtitles.append(subtitle);
            }
        }
    }
    file.close();
    qDebug() << "已加载字幕: " << srtFile;
    return subtitles;
}
