#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QVector>
#include <QStringList>

struct Subtitle {
    qint64 startTime; // 字幕开始时间（毫秒）
    qint64 endTime;   // 字幕结束时间（毫秒）
    QString text;     // 字幕文本
};

QVector<QString>* readPlayList();
void writePlayList(const QVector<QString>& playList);
QString getFileName(const QString& path);
QString getVideoInfo(const QString& path);
bool isValidVideoFile(const QString& path);
bool isValidStreamUrl(const QString& url);
QVector<Subtitle> loadSubtitles(const QString& srtFile);

#endif // UTILS_H
