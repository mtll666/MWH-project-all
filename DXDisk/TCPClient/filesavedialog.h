#ifndef FILESAVEDIALOG_H
#define FILESAVEDIALOG_H

#include <QWidget>
#include "protocol.h"

namespace Ui {
class FileSaveDialog;
}

class FileSaveDialog : public QWidget
{
    Q_OBJECT

public:
    explicit FileSaveDialog(QWidget *parent = nullptr);
    ~FileSaveDialog();
    static FileSaveDialog& getInstance();
    void setCurPath(QString path);      //设置当前路径
    QString getCurPathTemp();
    void refreshDialog();   //向服务器发送刷新请求
    void updateFileSaveDialog(std::shared_ptr<PDU> pdu);
    void setSaveFlag(bool ret);
    void closeEvent(QCloseEvent *event) override;

private slots:
    void on_returnPB_clicked();

    void on_FileInfoTreeW_doubleClicked(const QModelIndex &index);

    void on_createDirPB_clicked();

    void on_savePB_clicked();

    void on_cancelPB_clicked();

private:
    Ui::FileSaveDialog *ui;
    QString curPath;
    QString curPathTemp;
    bool isSaveFlag = true;        //保存标志,false为移动，true为保存
};

#endif // FILESAVEDIALOG_H
