#ifndef FILETRANSFERWIDGET_H
#define FILETRANSFERWIDGET_H

#include <QWidget>
#include <QTreeWidget>

namespace Ui {
class FileTransferWidget;
}

class FileTransferWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FileTransferWidget(QWidget *parent = nullptr);
    ~FileTransferWidget();
    QTreeWidget* getWidget();
    //创造一个FileTransferWidgetItem
    void createFTWItem(QString downloadFileName, int fileType, qint64 fileSize, QString status);
    //更新FileTransferWidgetItem的状态
    void updateFTWItem(qint64 recvedSize, QString process, QString speed, QString status);

private:
    Ui::FileTransferWidget *ui;
};

#endif // FILETRANSFERWIDGET_H
