#include "opewidget.h"
#include "tcpclient.h"

OpeWidget::OpeWidget(QWidget *parent)
    : QWidget{parent}
{
    this->setWindowTitle("DXDisk");

    m_pListW = new QListWidget(this);
    m_pListW->setFixedWidth(80);
    QListWidgetItem *pFriendItem = new QListWidgetItem("好友");
    pFriendItem->setIcon(QIcon(QPixmap(":/icon/home_item_icon/friend.png")));
    m_pListW->addItem(pFriendItem);

    QListWidgetItem *pWebDiskItem = new QListWidgetItem("网盘");
    pWebDiskItem->setIcon(QIcon(QPixmap(":/icon/home_item_icon/webdisk.png")));
    m_pListW->addItem(pWebDiskItem);

    QListWidgetItem *pFileTransferItem = new QListWidgetItem("传输");
    pFileTransferItem->setIcon(QIcon(QPixmap(":/icon/home_item_icon/filetransfer.png")));
    m_pListW->addItem(pFileTransferItem);

    QListWidgetItem *pFileRecvItem = new QListWidgetItem("接收");
    pFileRecvItem->setIcon(QIcon(QPixmap(":/icon/home_item_icon/recv.png")));
    m_pListW->addItem(pFileRecvItem);

    statusIcon = new QLabel;
    QIcon icon(":/icon/home_item_icon/connect.png"); // 使用资源文件路径或文件路径
    QPixmap pixmap = icon.pixmap(QSize(16, 16)); // 设置图标大小
    statusIcon->setPixmap(pixmap);

    statusText = new QLabel("已连接");
    statusText->setFixedWidth(50);
    statusText->setAlignment(Qt::AlignLeft);
    QPalette palette = statusText->palette();   // 创建一个 QPalette 对象
    palette.setColor(QPalette::WindowText, QColor(Qt::green));  // 将字体颜色设置为绿色
    statusText->setPalette(palette);    // 将 QPalette 应用到 QLabel

    QHBoxLayout *statusHBL = new QHBoxLayout;
    statusHBL->addWidget(statusIcon);
    statusHBL->addWidget(statusText);

    m_pReconnectPB = new QPushButton("重新连接");
    m_pReconnectPB->setFixedWidth(80);

    QVBoxLayout *statusVBL = new QVBoxLayout;
    statusVBL->addLayout(statusHBL);
    statusVBL->addWidget(m_pReconnectPB);

    QVBoxLayout *rightVBL = new QVBoxLayout;
    rightVBL->addWidget(m_pListW);
    rightVBL->addLayout(statusVBL);

    m_pFriend = new FriendWidget;
    m_pWebDisk = new WebDiskWiget;
    m_pFileTransfer = new FileTransferWidget;
    m_pFileRecv = new FileRecvDialog;

    m_pSW = new QStackedWidget;
    m_pSW->addWidget(m_pFriend);
    m_pSW->addWidget(m_pWebDisk);
    m_pSW->addWidget(m_pFileTransfer);
    m_pSW->addWidget(m_pFileRecv);

    QHBoxLayout *pMain = new QHBoxLayout;
    pMain->addLayout(rightVBL);
    pMain->addWidget(m_pSW);

    setLayout(pMain);
    connect(m_pListW,SIGNAL(currentRowChanged(int)),
            m_pSW,SLOT(setCurrentIndex(int)));
    connect(m_pListW,&QListWidget::itemClicked,
            this,&OpeWidget::fileRecvDIconChanged);
    connect(m_pReconnectPB,SIGNAL(clicked(bool)),
            this,SLOT(reconnect()));
}

OpeWidget &OpeWidget::getInstance()
{
    static OpeWidget instance;
    return instance;
}

QListWidget *OpeWidget::getListW()
{
    return m_pListW;
}

FriendWidget *OpeWidget::getFriendW()
{
    return m_pFriend;
}

WebDiskWiget *OpeWidget::getWebDiskW()
{
    return m_pWebDisk;
}

FileTransferWidget *OpeWidget::getFileTransferW()
{
    return m_pFileTransfer;
}

FileRecvDialog *OpeWidget::getFileRecvD()
{
    return m_pFileRecv;
}

QLabel *OpeWidget::getStatus()
{
    return statusText;
}

void OpeWidget::fileRecvDIconChanged(QListWidgetItem *item)
{
    if(item == m_pListW->item(3))
        item->setIcon(QIcon(QPixmap(":/icon/home_item_icon/recv.png")));
}

//重新连接服务器
void OpeWidget::reconnect()
{
    TCPClient::getInstance().on_connect_pb_clicked();
}
