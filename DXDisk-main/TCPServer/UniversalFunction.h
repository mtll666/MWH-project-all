#ifndef UNIVERSALFUNCTION_H
#define UNIVERSALFUNCTION_H

#include <QStringList>

//将一串中间带有'\0'的字符串转化为空格的函数
QString extractValidString(const char* input, int length);
//将字节B换算最高的单位的函数
QString byteConversion(qint64 size);
QString byteConversion(double size);
//判断文件类型的函数
int determineFileType(QString suffix);


#endif // UNIVERSALFUNCTION_H
