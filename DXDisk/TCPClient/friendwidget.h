#ifndef FRIENDWIDGET_H
#define FRIENDWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include "onlineusr.h"
#include "messagewidget.h"
#include "searchdialog.h"
#include <QStackedWidget>

class FriendWidget : public QWidget
{
    Q_OBJECT
    friend class SearchDialog;

public:
    explicit FriendWidget(QWidget *parent = nullptr);
    void showAllOnlineUsr(std::shared_ptr<PDU> pdu);    //显示在线用户函数
    void addFriend(QListWidgetItem *pItem);   //发送添加好友请求
    void delFriend(QListWidgetItem *pItem);   //发送删除好友请求
    void refreshFriend(std::shared_ptr<PDU> pdu);   //刷新好友列表
    //实时更新聊天信息
    void updateSendMsg();
    void updateRecvMsg(std::shared_ptr<PDU> pdu);
    void delCurFriInfo(QString delFriName);
    MessageWidget *publicChatW;  //群聊窗口
    QStringList *getOnlineFriend();

signals:

public slots:
    void on_onlineUsr_pb_clicked();   //处理显示/隐藏在线用户列表按键的函数
    void on_searchUsr_pb_clicked();   //查找用户
    void on_refFriend_pb_clicked();   //请求刷新好友列表
    void on_delFriend_pb_clicked();   //删除好友请求
    void on_sendMsg_pb_clicked();     //发送信息
    void privateChat(QListWidgetItem *item);         //私聊函数
    void on_clearMsg_pb_clicked();    //清空输入框信息
    void changeMsgWidget(QListWidgetItem *item);     //切换聊天窗口
    void on_publicChat_pb_clicked();    //切换群聊窗口

private:
    QStackedWidget *m_pMsgSW;   //聊天框
    bool PublicChatFlag = true; //群聊标志
    QMap<QString,MessageWidget*> m_pMsgWMap;   //聊天堆栈窗口
    QListWidget *m_pFriendLW;   //好友列表
    QStringList *m_pOnlineFriend;   //在线好友列表
    QListWidget *m_pFriStaLW;   //好友状态列表
    QListWidget *m_pRemMsgLW;   //消息提醒列表
    QMap<QString,uint> m_RemFlagMap; //消息是否已读标志
    QLineEdit *m_pInputMsgLE;   //消息输入框
    QPushButton *m_pClearMsgPB;    //清空输入框消息
    QPushButton *m_pSendMsgPB;    //发送输入框消息
    QCheckBox *m_pClearMsgCB;   //清空勾选框，发送完消息后是否清空消息

    QPushButton *m_pPublicChatPB;    //群聊
    QPushButton *m_pDelFriendPB;    //删除好友按钮
    QPushButton *m_pRefFriendPB;    //刷新好友列表
    QPushButton *m_pOnlineUsrPB;    //显示/隐藏在线用户列表
    QPushButton *m_pSearchUsrPB;    //搜索用户

    OnlineUsr *m_pOnlineUsrLW;      //在线用户列表
    QString temp;
};

#endif // FRIENDWIDGET_H
