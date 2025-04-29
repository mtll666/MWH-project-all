#ifndef UNIVERSALFUNCTION_H
#define UNIVERSALFUNCTION_H

#include <QStringList>
#include <QTreeWidget>

QString extractValidString(const char* input, int length);
QString byteConversion(qint64 size);
int determineFileType(QString suffix);
void setFileTypeIcon(QTreeWidgetItem *pFileInfoItem,int iFileType, int iconCol, int typeCol);

#endif // UNIVERSALFUNCTION_H
