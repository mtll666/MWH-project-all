#include "searchdialog.h"
#include "tcpclient.h"
#include <QBrush>
#include <QMessageBox>

SearchDialog::SearchDialog(QWidget *parent)
    :QDialog(parent)
{
    setWindowTitle("搜索结果");
    ResultLabel = new QLabel;
    UsrNameLabel = new QLabel("用户名");   //搜索用户名结果栏
    UsrStatusLabel = new QLabel("状态");  //搜索用户在线状态栏
    AddFriendPB = new QPushButton("添加好友");  //添加好友
    PriChatPB = new QPushButton("聊天");  //聊天
    DelFriendPB = new QPushButton("删除好友");  //删除好友
    UsrNameLW = new QListWidget;     //搜索结果列表
    UsrStatusLW = new QListWidget;      //在线状态列表
    UsrStatusLW->setEnabled(false);     //状态无法被选中

    //将用户名栏和列表垂直分布
    QVBoxLayout *UsrNameHBL = new QVBoxLayout;
    UsrNameHBL->addWidget(UsrNameLabel);
    UsrNameHBL->addWidget(UsrNameLW);

    //将用户状态栏和状态表垂直分布
    QVBoxLayout *UsrStatusHBL = new QVBoxLayout;
    UsrStatusHBL->addWidget(UsrStatusLabel);
    UsrStatusHBL->addWidget(UsrStatusLW);

    //将用户名结果栏和状态栏水平分布，比例20：1
    QHBoxLayout *LabelHBL = new QHBoxLayout;
    LabelHBL->addLayout(UsrNameHBL);
    LabelHBL->addLayout(UsrStatusHBL);
    LabelHBL->setStretchFactor(UsrNameHBL,20);
    LabelHBL->setStretchFactor(UsrStatusHBL,1);

    //将添加好友和私聊按钮水平分布
    QHBoxLayout *CtrlHBL = new QHBoxLayout;
    CtrlHBL->addWidget(AddFriendPB);
    CtrlHBL->addWidget(PriChatPB);
    CtrlHBL->addWidget(DelFriendPB);

    QVBoxLayout *MainVBL= new QVBoxLayout;
    MainVBL->addWidget(ResultLabel);
    MainVBL->addLayout(LabelHBL);
    MainVBL->addLayout(CtrlHBL);

    setLayout(MainVBL);

    connect(AddFriendPB,SIGNAL(clicked(bool)),
            this,SLOT(addFriend()));
    connect(PriChatPB,SIGNAL(clicked(bool)),
            this,SLOT(privateChat()));
    connect(DelFriendPB,SIGNAL(clicked(bool)),
            this,SLOT(delFriend()));
}

void SearchDialog::updateSearchDialog(std::shared_ptr<PDU> pdu)
{
    ResultLabel->setText(pdu->caData);   //搜索结果反馈
    UsrNameLW->clear();
    UsrStatusLW->clear();
    auto size = pdu->uiMsgLen/34;
    for(uint i = 0; i<size; ++i)
    {
        char name[32];
        char status[2];
        memcpy(name,(char*)pdu->caMsg+i*34,32);
        memcpy(status,(char*)pdu->caMsg+i*34+32,2);
        QListWidgetItem *itemName = new QListWidgetItem(name);
        if(strcmp(status,"1") == 0)
        {
            //将在线的用户字体调为绿色
            QListWidgetItem *itemStatus = new QListWidgetItem("在线");
            itemName->setForeground(QBrush(Qt::green));
            itemStatus->setForeground(QBrush(Qt::green));
            UsrStatusLW->addItem(itemStatus);
        }
        else UsrStatusLW->addItem("离线");
        UsrNameLW->addItem(itemName);
    }
}

SearchDialog &SearchDialog::getInstance()
{
    static SearchDialog instance;
    return instance;
}

SearchDialog::~SearchDialog()
{

}

void SearchDialog::addFriend()
{
    QListWidgetItem *pItem = UsrNameLW->currentItem();   //获取搜索用户列表选中的用户
    OpeWidget::getInstance().getFriendW()->addFriend(pItem);
}

void SearchDialog::privateChat()
{
    QListWidgetItem *pItem = UsrNameLW->currentItem();   //获取搜索用户列表选中的用户
    if(pItem == nullptr)return;
    QString perUsrName = pItem->text();     //获取要聊天好友的用户名,最长32字节
    if(perUsrName == TCPClient::getInstance().getLoginName())
    {
        QMessageBox::warning(this,"发起聊天失败","不能和自己聊天");
        return;
    }
    for(int i = 0; i < OpeWidget::getInstance().getFriendW()->m_pFriendLW->count(); ++i)
    {
        if (OpeWidget::getInstance().getFriendW()->m_pFriendLW->item(i)->text() == perUsrName)
        {
            auto pItem = OpeWidget::getInstance().getFriendW()->m_pFriendLW->item(i);
            OpeWidget::getInstance().getFriendW()->changeMsgWidget(pItem);
            OpeWidget::getInstance().getFriendW()->privateChat(pItem);
            SearchDialog::getInstance().close();
            return;
        }
    }
    QMessageBox::warning(this,"发起聊天失败","对方不是你的好友，请先添加好友再聊天");
}

void SearchDialog::delFriend()
{
    QListWidgetItem *pItem = UsrNameLW->currentItem();   //获取在线用户列表选中的用户
    if(pItem == nullptr)return;
    for(int i = 0; i < OpeWidget::getInstance().getFriendW()->m_pFriendLW->count(); ++i)
    {
        if (OpeWidget::getInstance().getFriendW()->m_pFriendLW->item(i)->text()
            == pItem->text())
        {
            OpeWidget::getInstance().getFriendW()->delFriend(pItem);
            return;
        }
    }
    QMessageBox::warning(this,"删除失败","对方不是你的好友");
}
