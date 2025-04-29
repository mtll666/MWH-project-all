#include "sharefilewidget.h"
#include "opewidget.h"
#include "tcpclient.h"
#include "opewidget.h"
#include <QMessageBox>

ShareFileWidget::ShareFileWidget(QWidget *parent)
    : QWidget(parent)
{
    m_pAllSelectPB = new QPushButton("全选");
    m_pCancelSelectPB = new QPushButton("取消选择");
    m_pRefreshPB = new QPushButton("刷新");
    m_pOkPB = new QPushButton("确认");
    m_pCancelPB = new QPushButton("取消");

    m_pSA = new QScrollArea;
    m_pSA->setWidgetResizable(true);    //使内部的小部件能够自动调整大小

    m_pOnlineFriendW = new QWidget;
    m_pOnlineFriendVBL = new QVBoxLayout(m_pOnlineFriendW);

    m_pOnlineFriendVBL->setSpacing(5);
    m_pOnlineFriendVBL->setContentsMargins(5,5,5,5);
    m_pOnlineFriendVBL->setAlignment(Qt::AlignTop);

    m_pButtonGroup = new QButtonGroup(m_pOnlineFriendW);
    m_pButtonGroup->setExclusive(false);    //设置为多选

    QHBoxLayout *selectHBL = new QHBoxLayout;
    selectHBL->addWidget(m_pAllSelectPB);
    selectHBL->addWidget(m_pCancelSelectPB);
    selectHBL->addStretch();
    selectHBL->addWidget(m_pRefreshPB);

    QHBoxLayout *ctrlHBL = new QHBoxLayout;
    ctrlHBL->addWidget(m_pOkPB);
    ctrlHBL->addStretch();
    ctrlHBL->addWidget(m_pCancelPB);

    QVBoxLayout *mainVBL = new QVBoxLayout;
    mainVBL->addLayout(selectHBL);
    mainVBL->addWidget(m_pSA);
    mainVBL->addLayout(ctrlHBL);

    setLayout(mainVBL);

    connect(m_pRefreshPB,SIGNAL(clicked(bool)),this,SLOT(on_refreshPB_clicked()));
    connect(m_pAllSelectPB,SIGNAL(clicked(bool)),this,SLOT(on_allSelectPB_clicked()));
    connect(m_pCancelSelectPB,SIGNAL(clicked(bool)),this,SLOT(on_cancelSelectPB_clicked()));
    connect(m_pOkPB,SIGNAL(clicked(bool)),this,SLOT(on_okPB_clicked()));
    connect(m_pCancelPB,SIGNAL(clicked(bool)),this,SLOT(on_cancelPB_clicked()));
}

ShareFileWidget &ShareFileWidget::getInstance()
{
    static ShareFileWidget instance;
    return instance;
}


void ShareFileWidget::updateShareFriList(QStringList *onlineFriList)
{
    //之前的在线好友列表,移除原来的列表
    QList<QAbstractButton*> preOnlFriList = m_pButtonGroup->buttons();
    if(!preOnlFriList.isEmpty())
    {
        // 遍历按钮列表，并从按钮组和布局中移除按钮
        for(QAbstractButton* button : preOnlFriList)
        {
            // 从按钮组中移除按钮
            m_pButtonGroup->removeButton(button);
            m_pOnlineFriendVBL->removeWidget(button);
            delete button;
        }
    }

    if(!onlineFriList->isEmpty())
    {
        for(int i = 0; i < onlineFriList->size(); ++i)
        {
            QCheckBox *pCB = new QCheckBox(onlineFriList->at(i));
            m_pOnlineFriendVBL->addWidget(pCB);
            m_pButtonGroup->addButton(pCB);
        }
        m_pSA->setWidget(m_pOnlineFriendW);
    }
    else qDebug() << "nullptr";
}

void ShareFileWidget::on_refreshPB_clicked()
{
    OpeWidget::getInstance().getFriendW()->on_refFriend_pb_clicked();
}

void ShareFileWidget::on_allSelectPB_clicked()
{
    QList<QAbstractButton*> cbList = m_pButtonGroup->buttons();
    for(auto cb : cbList)
        if(!cb->isChecked())cb->setChecked(true);
}

void ShareFileWidget::on_cancelSelectPB_clicked()
{
    QList<QAbstractButton*> cbList = m_pButtonGroup->buttons();
    for(auto cb : cbList)
        if(cb->isChecked())cb->setChecked(false);
}

void ShareFileWidget::on_okPB_clicked()
{
    QString senderName = TCPClient::getInstance().getLoginName();   //发送方名字
    QString curPath = TCPClient::getInstance().getCurPath();        //当前路径
    QString shareFileName = OpeWidget::getInstance().getWebDiskW()->getShareFileName(); //分享文件名
    QString shareFilePath = curPath + "/" + shareFileName;

    QList<QAbstractButton*> cbList = m_pButtonGroup->buttons();
    QStringList recverNameList; //接收方列表
    for(auto cb : cbList)
    {
        if(cb->isChecked())
        {
            recverNameList.append(cb->text());
        }
    }
    if(recverNameList.size())
    {
        int shareFilePathSizeUTF8 = shareFilePath.toUtf8().size();
        std::shared_ptr<PDU> pdu = mk_shared_PDU(shareFilePathSizeUTF8 + 1
                                                 + sizeof(recverNameList.size()) + 1
                                                 + recverNameList.size() * 64);
        pdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_REQUEST;
        strcpy(pdu->caData,senderName.toUtf8());    //发送方的名字存在caData中

        //分享文件路径存入caMsg第一部分
        memcpy(pdu->caMsg, shareFilePath.toUtf8(), shareFilePathSizeUTF8);
        qDebug() << "(ShareFileWidget::on_okPB_clicked)分享文件路径:" << pdu->caMsg;

        //接收方人数存入caMsg第二部分
        memcpy(pdu->caMsg + shareFilePathSizeUTF8 + 1,
               QString("%1").arg(recverNameList.size()).toStdString().c_str(),
               sizeof(recverNameList.size()));
        qDebug() << "(ShareFileWidget::on_okPB_clicked)分享人数:" << pdu->caMsg + shareFilePathSizeUTF8 + 1;

        //接收方名字存入caMsg第三部分
        for(int i = 0; i < recverNameList.size(); ++i)
        {
            strcpy(pdu->caMsg + shareFilePathSizeUTF8 + 1
                   + sizeof(recverNameList.size()) + 1
                   + i * 64,
                   recverNameList.at(i).toUtf8());
            qDebug() << "(ShareFileWidget::on_okPB_clicked)分享好友:"
                     << pdu->caMsg + shareFilePathSizeUTF8 + 1 + sizeof(recverNameList.size()) + 1 + i * 64;
        }

        TCPClient::getInstance().getTCPSocket().write((char*)pdu.get(),pdu->uiPDULen);
        QMessageBox::information(this,"分享文件","分享成功");
    }
    else QMessageBox::warning(this,"分享失败","请选择分享对象");

}

void ShareFileWidget::on_cancelPB_clicked()
{
    this->close();
    QList<QAbstractButton*> cbList = m_pButtonGroup->buttons();
    for(auto cb : cbList)
        if(cb->isChecked())cb->setChecked(false);
}


