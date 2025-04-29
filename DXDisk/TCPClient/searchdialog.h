#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QTableWidget>
#include "protocol.h"

class SearchDialog : public QDialog
{
    Q_OBJECT
public:
    SearchDialog(QWidget *parent = nullptr);
    void updateSearchDialog(std::shared_ptr<PDU> pdu);  //搜索显示
    static SearchDialog &getInstance();
    ~SearchDialog();

private slots:
    void addFriend();   //添加好友
    void privateChat(); //聊天
    void delFriend();   //删除好友

private:
    QLabel *ResultLabel;   //搜索结果
    QLabel *UsrNameLabel;   //搜索用户名结果栏
    QLabel *UsrStatusLabel;  //搜索用户在线状态栏
    QListWidget *UsrNameLW;    //用户名列表
    QListWidget *UsrStatusLW;  //在线状态
    QPushButton *AddFriendPB;   //添加好友
    QPushButton *PriChatPB;     //聊天
    QPushButton *DelFriendPB;     //删除好友
};

#endif // SEARCHDIALOG_H
