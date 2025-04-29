#include "onlineusr.h"
#include "ui_onlineusr.h"
#include "tcpclient.h"

OnlineUsr::OnlineUsr(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::OnlineUsr)
{
    ui->setupUi(this);
}

OnlineUsr::~OnlineUsr()
{
    delete ui;
}

void OnlineUsr::showUsr(std::shared_ptr<PDU> pdu)
{
    char OnlineUsrName[32];
    uint usrSize = pdu->uiMsgLen/32;
    ui->online_usr_lw->clear();     //清空在线用户列表
    for(uint i=0; i<usrSize; ++i)
    {
        memcpy(OnlineUsrName,(char*)(pdu->caMsg)+i*32,32);
        ui->online_usr_lw->addItem(OnlineUsrName);
    }
}


//点击“添加好友”按钮的处理函数
void OnlineUsr::on_addFriend_pb_clicked()
{
    QListWidgetItem *pItem = ui->online_usr_lw->currentItem();   //获取在线用户列表选中的用户
    OpeWidget::getInstance().getFriendW()->addFriend(pItem);
}

