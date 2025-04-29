#ifndef OPEWIDGET_H
#define OPEWIDGET_H

#include <QWidget>
#include <QListWidget>
#include "friendwidget.h"
#include "webdiskwiget.h"
#include "filetransferwidget.h"
#include "filerecvdialog.h"
#include <QStackedWidget>
#include <QPushButton>

class OpeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OpeWidget(QWidget *parent = nullptr);
    static OpeWidget& getInstance();
    QListWidget *getListW();
    FriendWidget *getFriendW();
    WebDiskWiget *getWebDiskW();
    FileTransferWidget *getFileTransferW();
    FileRecvDialog *getFileRecvD();
    QLabel *getStatus();

signals:

public slots:
    void fileRecvDIconChanged(QListWidgetItem *item);
    void reconnect();

private:
    QLabel *statusIcon;
    QLabel *statusText;
    QPushButton *m_pReconnectPB;

    QListWidget *m_pListW;
    FriendWidget *m_pFriend;    //好友窗口
    WebDiskWiget *m_pWebDisk;   //网盘窗口
    FileTransferWidget *m_pFileTransfer;    //传输窗口
    FileRecvDialog *m_pFileRecv;    //文件接收窗口

    QStackedWidget *m_pSW;      //堆栈窗口
};

#endif // OPEWIDGET_H
