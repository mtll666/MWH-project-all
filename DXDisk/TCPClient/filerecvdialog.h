#ifndef FILERECVDIALOG_H
#define FILERECVDIALOG_H

#include <QWidget>
#include <QTreeWidget>
#include "protocol.h"

namespace Ui {
class FileRecvDialog;
}

class FileRecvDialog : public QWidget
{
    Q_OBJECT

public:
    explicit FileRecvDialog(QWidget *parent = nullptr);
    ~FileRecvDialog();
    void isEmpty();
    void addFileRecvDialog(std::shared_ptr<PDU> pdu);
    QStringList &getShareFilePathList();
    int getSaveFileIndex();
    void deleteFileInfoItem(int index);
    void releaseAgreePB();

private slots:
    void on_refusePB_clicked();

    void on_agreePB_clicked();

private:
    Ui::FileRecvDialog *ui;
    QStringList shareFilePathList;
    int saveFileIndex = -1;      //选中文件列表的下标
};

#endif // FILERECVDIALOG_H
