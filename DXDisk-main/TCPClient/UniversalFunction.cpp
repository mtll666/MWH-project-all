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

void setFileTypeIcon(QTreeWidgetItem *pFileInfoItem,int iFileType, int iconCol, int typeCol)
{
    switch(iFileType)
    {
    case 0:
    {
        pFileInfoItem->setIcon(iconCol,(QPixmap(":/icon/file_type_icon/dir.png")));
        pFileInfoItem->setText(typeCol,"文件夹");
        break;
    }
    case 1:
    {
        pFileInfoItem->setIcon(iconCol,(QPixmap(":/icon/file_type_icon/txt.png")));
        pFileInfoItem->setText(typeCol,"文本文档");
        break;
    }
    case 2:
    {
        pFileInfoItem->setIcon(iconCol,(QPixmap(":/icon/file_type_icon/picture.png")));
        pFileInfoItem->setText(typeCol,"图片文件");
        break;
    }
    case 3:
    {
        pFileInfoItem->setIcon(iconCol,(QPixmap(":/icon/file_type_icon/audio.png")));
        pFileInfoItem->setText(typeCol,"音频文件");
        break;
    }
    case 4:
    {
        pFileInfoItem->setIcon(iconCol,(QPixmap(":/icon/file_type_icon/video.png")));
        pFileInfoItem->setText(typeCol,"视频文件");
        break;
    }
    case 5:
    {
        pFileInfoItem->setIcon(iconCol,(QPixmap(":/icon/file_type_icon/exe.png")));
        pFileInfoItem->setText(typeCol,"可执行文件");
        break;

    }
    case 6:
    {
        pFileInfoItem->setIcon(iconCol,(QPixmap(":/icon/file_type_icon/gif.png")));
        pFileInfoItem->setText(typeCol,"动图");
        break;
    }
    case 7:
    {
        pFileInfoItem->setIcon(iconCol,(QPixmap(":/icon/file_type_icon/ppt.png")));
        pFileInfoItem->setText(typeCol,"演示文稿");
        break;
    }
    case 8:
    {
        pFileInfoItem->setIcon(iconCol,(QPixmap(":/icon/file_type_icon/word.png")));
        pFileInfoItem->setText(typeCol,"文档");
        break;
    }
    case 9:
    {
        pFileInfoItem->setIcon(iconCol,(QPixmap(":/icon/file_type_icon/xlsx.png")));
        pFileInfoItem->setText(typeCol,"工作表");
        break;
    }
    case 10:
    {
        pFileInfoItem->setIcon(iconCol,(QPixmap(":/icon/file_type_icon/pdf.png")));
        pFileInfoItem->setText(typeCol,"可携带文件");
        break;
    }
    case 11:
    {
        pFileInfoItem->setIcon(iconCol,(QPixmap(":/icon/file_type_icon/zip.png")));
        pFileInfoItem->setText(typeCol,"压缩文件");
        break;
    }
    default:
    {
        pFileInfoItem->setIcon(iconCol,(QPixmap(":/icon/file_type_icon/unknow.png")));
        pFileInfoItem->setText(typeCol,"未知格式文件");
        break;
    }
    }
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
