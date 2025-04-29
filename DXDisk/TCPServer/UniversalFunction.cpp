#include "UniversalFunction.h"

QString extractValidString(const char* input, int length)
{
    QStringList strList;
    QString result;
    int begin = 0, end = 0;
    for (; end < length; ++end)
    {
        if(input[end] == '\0' && input[end+1] != '\0')
            begin = end +1;
        if(input[end] != '\0' && input[end+1] == '\0')
        {
            strList.append(input + begin);
            begin = end + 1;
        }
    }
    result = strList.join(" ");
    return result;
}

QString byteConversion(qint64 size)
{
    QStringList unit = {"B","KB","MB","GB","TB","PB","EB","ZB","YB"};
    int unitIndex = 0;
    double dSize = size;
    while(dSize/1024 >= 1)
    {
        dSize /= 1024.0;
        ++unitIndex;
    }
    QString strSize;
    if(unitIndex == 0)strSize = QString::number(dSize,'f',0);
    else strSize = QString::number(dSize,'f',2);
    return strSize + unit[unitIndex];
}

QString byteConversion(double size)
{
    QStringList unit = {"B","KB","MB","GB","TB","PB","EB","ZB","YB"};
    int unitIndex = 0;

    while(size/1024.0 >= 1)
    {
        size /= 1024.0;
        ++unitIndex;
    }
    QString strSize = QString::number(size,'f',2) + unit[unitIndex];
    return strSize;
}

int determineFileType(QString suffix)
{
    if(suffix == "txt" || suffix == "log")
        return 1;   //文本文件类型
    else if(suffix == "jpg" || suffix == "png" || suffix == "jpeg" || suffix == "bmp")
        return 2;   //图片文件类型
    else if(suffix == "mp3")
        return 3;   //音频文件类型
    else if(suffix == "mp4")
        return 4;   //视频文件类型
    else if(suffix == "exe")
        return 5;   //可执行文件类型
    else if(suffix == "gif")
        return 6;   //动图文件类型
    else if(suffix == "ppt" || suffix == "pptx")
        return 7;   //PPT文件类型
    else if(suffix == "doc" || suffix == "docx")
        return 8;   //word文件类型
    else if(suffix == "xls" || suffix == "xlsx")
        return 9;   //表格文件类型
    else if(suffix == "pdf")
        return 10;   //pdf文件类型
    else if(suffix == "zip" || suffix == "rar" || suffix == "7z" || suffix == "jar")
        return 11;   //压缩包文件类型
    else return 12; //未知类型
}
