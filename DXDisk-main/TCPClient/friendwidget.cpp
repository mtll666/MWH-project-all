#include "friendwidget.h"
#include "protocol.h"
#include "tcpclient.h"
#include "sharefilewidget.h"
#include <QInputDialog>
#include <QMessageBox>


FriendWidget::FriendWidget(QWidget *parent)
    : QWidget{parent}
{
    m_pMsgSW = new QStackedWidget;
    publicChatW = new MessageWidget;
    publicChatW->perNameTE->setText("Public Chat");
    m_pMsgSW->addWidget(publicChatW);

    m_pFriendLW = new QListWidget;
    m_pOnlineFriend = new QStringList;
    m_pFriStaLW = new QListWidget;
    m_pFriStaLW->setFixedWidth(80);
    m_pRemMsgLW = new QListWidget;
    m_pRemMsgLW->setFixedWidth(100);
    m_pFriStaLW->setEnabled(false);     //状态无法被选中
    m_pRemMsgLW->setEnabled(false);
    m_pInputMsgLE = new QLineEdit;
    m_pClearMsgPB = new QPushButton("清空");
    m_pSendMsgPB = new QPushButton("发送");
    m_pClearMsgCB = new QCheckBox;
    m_pClearMsgCB->setChecked(true);    //清空勾选框默认被勾选

    m_pPublicChatPB = new QPushButton("群聊");
    m_pDelFriendPB = new QPushButton("删除好友");
    m_pRefFriendPB = new QPushButton("刷新好友");
    m_pOnlineUsrPB = new QPushButton("在线用户");
    m_pSearchUsrPB = new QPushButton("搜索用户");

    m_pOnlineUsrLW =new OnlineUsr;

    //将以下五个按钮垂直分布
    QVBoxLayout *pRightPBVBL = new QVBoxLayout;
    pRightPBVBL->addWidget(m_pPublicChatPB);
    pRightPBVBL->addWidget(m_pDelFriendPB);
    pRightPBVBL->addWidget(m_pRefFriendPB);
    pRightPBVBL->addWidget(m_pOnlineUsrPB);
    pRightPBVBL->addWidget(m_pSearchUsrPB);

    //将消息输入框、清空键、清空勾选框、发送键水平分布
    QHBoxLayout *pMsgHBL = new QHBoxLayout;
    pMsgHBL->addWidget(m_pInputMsgLE);
    pMsgHBL->addWidget(m_pClearMsgPB);
    pMsgHBL->addWidget(m_pClearMsgCB);
    pMsgHBL->addWidget(m_pSendMsgPB);

    //将聊天框和消息输入框、发送键垂直分布
    QVBoxLayout *pMsgVBL = new QVBoxLayout;
    pMsgVBL->addWidget(m_pMsgSW);
    pMsgVBL->addLayout(pMsgHBL);

    //将聊天框、好友列表、操作按钮、在线用户列表水平分布
    QHBoxLayout *pMainHBL = new QHBoxLayout;
    pMainHBL->addLayout(pMsgVBL);
    pMainHBL->addWidget(m_pFriendLW);
    pMainHBL->addWidget(m_pFriStaLW);
    pMainHBL->addWidget(m_pRemMsgLW);
    pMainHBL->addLayout(pRightPBVBL);
    pMainHBL->setStretchFactor(pMsgVBL,5);
    pMainHBL->setStretchFactor(m_pFriendLW,3);
    pMainHBL->setStretchFactor(m_pFriStaLW,1);
    pMainHBL->setStretchFactor(m_pRemMsgLW,2);
    pMainHBL->setStretchFactor(pRightPBVBL,1);

    m_pOnlineUsrLW->hide(); //在线用户列表默认隐藏

    setLayout(pMainHBL);
    connect(m_pOnlineUsrPB,SIGNAL(clicked(bool)),
            this,SLOT(on_onlineUsr_pb_clicked()));
    connect(m_pSearchUsrPB,SIGNAL(clicked(bool)),
            this,SLOT(on_searchUsr_pb_clicked()));
    connect(m_pRefFriendPB,SIGNAL(clicked(bool)),
            this,SLOT(on_refFriend_pb_clicked()));
    connect(m_pDelFriendPB,SIGNAL(clicked(bool)),
            this,SLOT(on_delFriend_pb_clicked()));
    connect(m_pSendMsgPB,SIGNAL(clicked(bool)),
            this,SLOT(on_sendMsg_pb_clicked()));
    connect(m_pFriendLW,&QListWidget::itemClicked,
            this,&FriendWidget::privateChat);
    connect(m_pFriendLW,&QListWidget::itemClicked,
            this,&FriendWidget::changeMsgWidget);
    connect(m_pClearMsgPB,SIGNAL(clicked(bool)),
            this,SLOT(on_clearMsg_pb_clicked()));
    connect(m_pPublicChatPB,SIGNAL(clicked(bool)),
            this,SLOT(on_publicChat_pb_clicked()));
}

void FriendWidget::showAllOnlineUsr(std::shared_ptr<PDU> pdu)
{
    m_pOnlineUsrLW->showUsr(pdu);
}

void FriendWidget::addFriend(QListWidgetItem *pItem)
{
    if(pItem == nullptr)return;
    QString perUsrName = pItem->text();     //获取要添加好友的用户名,最长32字节
    QString myName = TCPClient::getInstance().getLoginName();   //获取自己的名字，最长32字节
    std::shared_ptr<PDU> pdu = mk_shared_PDU(0);
    pdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_REQUEST;
    memcpy(pdu->caData,perUsrName.toUtf8(),perUsrName.toUtf8().size());
    memcpy(pdu->caData+32,myName.toUtf8(),myName.toUtf8().size());
    TCPClient::getInstance().getTCPSocket().write((char*)pdu.get(),pdu->uiPDULen);
}

void FriendWidget::delFriend(QListWidgetItem *pItem)
{
    if(pItem != nullptr)
    {
        QString delFriendName = pItem->text();     //获取要删除好友的用户名,最长32字节
        QString myName = TCPClient::getInstance().getLoginName();  //获取当前客户端用户名
        //提示是否确定删除
        auto ret = QMessageBox::information(this,"删除好友",QString
                    ("Are you sure you want to delete friend [%1] ?").arg(delFriendName),
                                            QMessageBox::Yes,QMessageBox::No);
        if(ret == QMessageBox::Yes)
        {
            std::shared_ptr<PDU> pdu = mk_shared_PDU(0);
            pdu->uiMsgType = ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST;
            memcpy(pdu->caData,delFriendName.toUtf8(),delFriendName.toUtf8().size());
            memcpy(pdu->caData+32,myName.toUtf8(),myName.toUtf8().size());
            TCPClient::getInstance().getTCPSocket().write((char*)pdu.get(),pdu->uiPDULen);
        }
    }
}

void FriendWidget::refreshFriend(std::shared_ptr<PDU> pdu)
{
    //qDebug() << "(FriendWidget::refreshFriend)刷新好友:";
    m_pSendMsgPB->setEnabled(false);
    m_pFriendLW->clear();
    m_pOnlineFriend->clear();
    m_pFriStaLW->clear();
    m_pRemMsgLW->clear();
    auto size = pdu->uiMsgLen/34;
    //qDebug() << "(FriendWidget::refreshFriend)好友个数:" << size;
    for(uint i = 0; i<size; ++i)
    {
        char name[32];
        char status[2];
        memcpy(name,(char*)pdu->caMsg+i*34,32);
        memcpy(status,(char*)pdu->caMsg+i*34+32,2);

        //若状态MAP中没有该用户name,那么定义该用户并初始化
        if(!m_RemFlagMap.contains(name))m_RemFlagMap[name] = 0;

        QListWidgetItem *itemName = new QListWidgetItem(name);
        QListWidgetItem *itemStatus = new QListWidgetItem("OFFLINE");
        QListWidgetItem *itemReminder = new QListWidgetItem("No new news.");
        itemStatus->setTextAlignment(Qt::AlignHCenter);
        if(strcmp(status,"1") == 0)
        {
            m_pOnlineFriend->append(name);
            //将在线的用户字体调为绿色
            itemStatus ->setText("ONLINE");
            itemName->setForeground(QBrush(Qt::green));
            itemStatus->setForeground(QBrush(Qt::green));
        }
        m_pFriendLW->addItem(itemName);
        m_pFriStaLW->addItem(itemStatus);
        if(m_RemFlagMap[name] > 0)
        {
            itemReminder->setText(QString("Have %1 new news.").arg(m_RemFlagMap[name]));
            itemReminder->setForeground(QBrush(Qt::red));
        }
        m_pRemMsgLW->addItem(itemReminder);

        //若聊天窗口MAP中没有该用户name,那么定义该用户并初始化
        if(!m_pMsgWMap.contains(name))
        {
            MessageWidget *msgW = new MessageWidget();
            msgW->perNameTE->setText(QString("Chating with [%1]").arg(name));
            m_pMsgSW->addWidget(msgW);
            m_pMsgWMap[name] = msgW;
        }
    }
    ShareFileWidget::getInstance().updateShareFriList(m_pOnlineFriend);
}

void FriendWidget::updateSendMsg()
{
    QString name = TCPClient::getInstance().getLoginName();
    const QString &msg = QString("[%1] said: %2")
                             .arg(name)
                             .arg(temp);
    MessageWidget *curMsgW = (MessageWidget*)m_pMsgSW->currentWidget();
    curMsgW->setContent(msg);
}

void FriendWidget::updateRecvMsg(std::shared_ptr<PDU> pdu)
{
    char perName[32];
    memcpy(perName,pdu->caData+32,32);
    const QString &msg = QString("[%1] said: %2")
                             .arg(pdu->caData+32)
                             .arg((char*)pdu->caMsg);
    // qDebug() << "(FriendWidget::updateRecvMsg)谁发消息来了:" << perName;
    // qDebug() << "(FriendWidget::updateRecvMsg)正在聊天对象:"
    //          << (m_pFriendLW->currentRow() == -1 ?
    //              "无" : m_pFriendLW->currentItem()->text());
    if(pdu->uiMsgType == ENUM_MSG_TYPE_SENDPUBLICMSG_REQUEST)
    {
        publicChatW->setContent(msg);
    }
    else if(pdu->uiMsgType == ENUM_MSG_TYPE_SENDMSG_REQUEST)
    {
        if(m_pFriendLW->currentRow() == -1 || perName != m_pFriendLW->currentItem()->text())
        {
            ++m_RemFlagMap[perName];
            for(int i = 0; i < m_pFriendLW->count(); ++i)
            {
                QListWidgetItem *item = m_pFriendLW->item(i);
                if (item->text() == perName)
                {
                    m_pRemMsgLW->item(i)->setText(QString("Have %1 new news.")
                                                      .arg(m_RemFlagMap[perName]));
                    m_pRemMsgLW->item(i)->setForeground(QBrush(Qt::red));
                }
            }
        }
        MessageWidget *perMsgW = m_pMsgWMap[perName];
        perMsgW->setContent(msg);
    }
}

void FriendWidget::delCurFriInfo(QString delFriName)
{
    delete m_pMsgWMap[delFriName];
    m_pMsgWMap.remove(delFriName);
    m_RemFlagMap.remove(delFriName);
}

QStringList *FriendWidget::getOnlineFriend()
{
    return m_pOnlineFriend;
}

void FriendWidget::on_onlineUsr_pb_clicked()
{
    if(m_pOnlineUsrLW->isHidden())
    {
        m_pOnlineUsrLW->show();
        std::shared_ptr<PDU> pdu = mk_shared_PDU(0);
        pdu->uiMsgType = ENUM_MSG_TYPE_ALL_ONLINEUSR_REQUEST;
        TCPClient::getInstance().getTCPSocket().write((char*)pdu.get(),pdu->uiPDULen);
    }
    else m_pOnlineUsrLW->hide();
}

void FriendWidget::on_searchUsr_pb_clicked()
{
    bool isOk;
    QString searchName = QInputDialog::getText(this,"搜索用户","输入要搜索的用户名",
                            QLineEdit::Normal,"",&isOk);
    if(!searchName.isEmpty())
    {
        std::shared_ptr<PDU> pdu = mk_shared_PDU(0);
        pdu->uiMsgType = ENUM_MSG_TYPE_SEARCH_USR_REQUEST;
        memcpy(pdu->caData,searchName.toUtf8(),searchName.toUtf8().size());
        //获取自己的名字，最长32字节
        QString myName = TCPClient::getInstance().getLoginName();
        memcpy(pdu->caData+32,myName.toUtf8(),myName.toUtf8().size());
        TCPClient::getInstance().getTCPSocket().write((char*)pdu.get(),pdu->uiPDULen);
    }
    else if(isOk) QMessageBox::warning(this,"搜索结果","用户名为空");
}

void FriendWidget::on_refFriend_pb_clicked()
{
    QString myName = TCPClient::getInstance().getLoginName();   //获取当前客户端用户名
    std::shared_ptr<PDU> pdu = mk_shared_PDU(0);
    pdu->uiMsgType = ENUM_MSG_TYPE_REFRESH_FRIEND_REQUEST;
    memcpy(pdu->caData,myName.toUtf8(),myName.toUtf8().size());
    TCPClient::getInstance().getTCPSocket().write((char*)pdu.get(),pdu->uiPDULen);
}

void FriendWidget::on_delFriend_pb_clicked()
{
    QListWidgetItem *pItem = m_pFriendLW->currentItem();   //获取在线用户列表选中的用户
    delFriend(pItem);
}

void FriendWidget::on_sendMsg_pb_clicked()
{
    QString msg = m_pInputMsgLE->text();
    if(!msg.isEmpty())
    {
        temp = msg;
        std::shared_ptr<PDU> pdu = mk_shared_PDU(msg.toUtf8().size()+1);
        QString myName = TCPClient::getInstance().getLoginName();
        memcpy(pdu->caData+32,myName.toUtf8(),myName.toUtf8().size());
        if(PublicChatFlag)
        {
            pdu->uiMsgType = ENUM_MSG_TYPE_SENDPUBLICMSG_REQUEST;
        }
        else
        {
            QString perName = m_pFriendLW->currentItem()->text();
            pdu->uiMsgType = ENUM_MSG_TYPE_SENDMSG_REQUEST;
            memcpy(pdu->caData,perName.toUtf8(),perName.toUtf8().size());
        }
        strncpy((char*)pdu->caMsg,msg.toUtf8(),msg.toUtf8().size());
        TCPClient::getInstance().getTCPSocket().write((char*)pdu.get(),pdu->uiPDULen);
    }
    if(m_pClearMsgCB->isChecked())m_pInputMsgLE->clear();
}

void FriendWidget::privateChat(QListWidgetItem *item)
{
    PublicChatFlag = false;
    int friendRow = 0;
    if(item == m_pFriendLW->currentItem())friendRow = m_pFriendLW->currentRow();
    else
    {
        for (int i = 0; i < m_pFriendLW->count(); ++i)
        {
            if (item == m_pFriendLW->item(i))
            {
                friendRow = i;
                break;
            }
        }
    }

    auto friendStatus = m_pFriStaLW->item(friendRow)->text();

    if(friendStatus == "ONLINE")
        m_pSendMsgPB->setEnabled(true);
    else
        m_pSendMsgPB->setEnabled(false);

    if(m_RemFlagMap[item->text()])
    {
        m_RemFlagMap[item->text()] = 0;
        m_pRemMsgLW->item(friendRow)->setText("No new news.");
        m_pRemMsgLW->item(friendRow)->setForeground(QBrush());
    }
}

void FriendWidget::on_clearMsg_pb_clicked()
{
    m_pInputMsgLE->clear();
}

void FriendWidget::changeMsgWidget(QListWidgetItem *item)
{
    QString name = item->text();
    if(!name.isEmpty() && m_pMsgWMap.contains(name))
        m_pMsgSW->setCurrentWidget(m_pMsgWMap[name]);
}

void FriendWidget::on_publicChat_pb_clicked()
{
    PublicChatFlag = true;
    m_pSendMsgPB->setEnabled(true);
    m_pMsgSW->setCurrentWidget(publicChatW);
}
