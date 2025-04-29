#include "tcpclient.h"
#include "ui_tcpclient.h"
#include <QMessageBox>
#include <QHostAddress>
#include <QFileDialog>
#include <QDebug>
#include "opewidget.h"
#include "filesavedialog.h"

// 构造函数，初始化 UI 和信号槽
TCPClient::TCPClient(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TCPClient)
    , m_elapsedTimer(new QElapsedTimer)
{
    ui->setupUi(this);
    loadConfig();
    connect(&m_tcpSocket, &QTcpSocket::connected, this, &TCPClient::showConnect);
    connect(&m_tcpSocket, &QTcpSocket::readyRead, this, &TCPClient::recvMsg);
    connect(&m_tcpSocket, &QTcpSocket::disconnected, this, &TCPClient::clientOffline);
}

// 析构函数，清理 UI 和计时器
TCPClient::~TCPClient()
{
    delete m_elapsedTimer;
    delete ui;
}

// 单例模式
TCPClient &TCPClient::getInstance()
{
    static TCPClient instance;
    return instance;
}

// 加载配置文件
void TCPClient::loadConfig()
{
    QFile file(":/client.config");
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray baData = file.readAll();
        QString strData = QString(baData).trimmed();
        file.close();
        QStringList strList = strData.split("\n");
        m_strIP = strList[0].trimmed();
        m_usPort = strList[1].trimmed().toUShort();
        qDebug() << "(TCPClient::loadConfig) 客户端配置成功: ip:" << m_strIP << " port:" << m_usPort;
    } else {
        QMessageBox::critical(this, "打开配置", "无法打开 client.config 文件");
    }
}

// 获取 TCP 套接字
QTcpSocket &TCPClient::getTCPSocket()
{
    return m_tcpSocket;
}

// 获取登录用户名
QString TCPClient::getLoginName()
{
    return m_loginName;
}

// 获取根路径
QString TCPClient::getRootPath()
{
    return m_rootPath;
}

// 获取当前路径
QString TCPClient::getCurPath()
{
    return m_curPath;
}

// 设置当前路径临时值
void TCPClient::setCurPathTemp(QString strCurPathTemp)
{
    m_curPathTemp = strCurPathTemp;
}

// 获取文件接收对象
FileRecv &TCPClient::getFileRecv()
{
    return m_fileRecv;
}

// 设置文件接收状态
void TCPClient::setFileRecv()
{
    m_fileRecv.recvingFlag = true;
    m_fileRecv.recvedSize = 0;
}

// 启动计时器
void TCPClient::startElapsedTimer()
{
    m_elapsedTimer->start();
}

// 显示连接成功
void TCPClient::showConnect()
{
    ui->status_lab->setText("连接成功");
    QPalette palette = ui->status_lab->palette();
    palette.setColor(QPalette::WindowText, Qt::green);
    ui->status_lab->setPalette(palette);
    OpeWidget::getInstance().getStatus()->setText("已连接");
    QPalette palette2 = OpeWidget::getInstance().getStatus()->palette();
    palette2.setColor(QPalette::WindowText, Qt::green);
    OpeWidget::getInstance().getStatus()->setPalette(palette2);
}

// 处理客户端下线
void TCPClient::clientOffline()
{
    ui->status_lab->setText("连接断开");
    QPalette palette = ui->status_lab->palette();
    palette.setColor(QPalette::WindowText, Qt::red);
    ui->status_lab->setPalette(palette);
    OpeWidget::getInstance().getStatus()->setText("未连接");
    QPalette palette2 = OpeWidget::getInstance().getStatus()->palette();
    palette2.setColor(QPalette::WindowText, Qt::red);
    OpeWidget::getInstance().getStatus()->setPalette(palette2);
}

// 连接服务器
void TCPClient::on_connect_pb_clicked()
{
    QString ip = ui->ip_le->text();
    quint16 port = ui->port_le->text().toUShort();
    m_tcpSocket.connectToHost(QHostAddress(ip), port);
}

// 注册
void TCPClient::on_regist_pb_clicked()
{
    QString strName = ui->name_le->text();
    QString strPwd = ui->pwd_le->text();
    if (strName.isEmpty() || strPwd.isEmpty()) {
        QMessageBox::warning(this, "注册", "用户名或密码不能为空");
        return;
    }
    std::shared_ptr<PDU> pdu = mk_shared_PDU(0);
    pdu->uiMsgType = ENUM_MSG_TYPE_REGIST_REQUEST;
    strncpy(pdu->caData, strName.toUtf8(), 32);
    strncpy(pdu->caData + 32, strPwd.toUtf8(), 32);
    m_tcpSocket.write((char*)pdu.get(), pdu->uiPDULen);
}

// 登录
void TCPClient::on_login_pb_clicked()
{
    QString strName = ui->name_le->text();
    QString strPwd = ui->pwd_le->text();
    if (strName.isEmpty() || strPwd.isEmpty()) {
        QMessageBox::warning(this, "登录", "用户名或密码不能为空");
        return;
    }
    m_loginName = strName;
    std::shared_ptr<PDU> pdu = mk_shared_PDU(0);
    pdu->uiMsgType = ENUM_MSG_TYPE_LOGIN_REQUEST;
    strncpy(pdu->caData, strName.toUtf8(), 32);
    strncpy(pdu->caData + 32, strPwd.toUtf8(), 32);
    m_tcpSocket.write((char*)pdu.get(), pdu->uiPDULen);
}

// 接收消息
void TCPClient::recvMsg()
{
    if (!m_fileRecv.recvingFlag) {
        while (m_tcpSocket.bytesAvailable() >= static_cast<qint64>(sizeof(uint))) {
            uint uiPDULen = 0;
            m_tcpSocket.peek((char*)&uiPDULen, sizeof(uint));
            if (m_tcpSocket.bytesAvailable() < static_cast<qint64>(uiPDULen)) {
                qDebug() << "recvMsg: Incomplete PDU, waiting for more data";
                return;
            }
            QByteArray buffer = m_tcpSocket.read(uiPDULen);
            PDU *pdu = (PDU*)buffer.data();
            std::shared_ptr<PDU> sharedPdu = mk_shared_PDU(pdu->uiMsgLen);
            memcpy(sharedPdu.get(), pdu, uiPDULen);
            showPDUInfo(sharedPdu);
            try {
                switch (sharedPdu->uiMsgType) {
                case ENUM_MSG_TYPE_REGIST_RESPOND: {
                    if (strcmp(sharedPdu->caData, REGIST_OK) == 0) {
                        QMessageBox::information(this, "注册", "注册成功");
                    } else {
                        QMessageBox::warning(this, "注册", "注册失败: 用户名已存在");
                    }
                    break;
                }
                case ENUM_MSG_TYPE_LOGIN_RESPOND: {
                    if (strcmp(sharedPdu->caData, LOGIN_OK) == 0) {
                        m_rootPath = QString("./UserWebDisk/%1").arg(m_loginName);
                        m_curPath = m_rootPath;
                        OpeWidget::getInstance().show();
                        this->hide();
                        OpeWidget::getInstance().getWebDiskW()->refreshDir();
                    } else if (strcmp(sharedPdu->caData, LOGIN_FAILED_NOEXIST) == 0) {
                        QMessageBox::warning(this, "登录", "登录失败: 用户不存在");
                    } else if (strcmp(sharedPdu->caData, LOGIN_FAILED_PWDERROR) == 0) {
                        QMessageBox::warning(this, "登录", "登录失败: 密码错误");
                    } else {
                        QMessageBox::warning(this, "登录", "登录失败: 用户已在线");
                    }
                    break;
                }
                case ENUM_MSG_TYPE_ALL_ONLINEUSR_RESPOND: {
                    OpeWidget::getInstance().getFriendW()->showAllOnlineUsr(sharedPdu);
                    break;
                }
                case ENUM_MSG_TYPE_SEARCH_USR_RESPOND: {
                    SearchDialog::getInstance().updateSearchDialog(sharedPdu);
                    SearchDialog::getInstance().show();
                    break;
                }
                case ENUM_MSG_TYPE_ADD_FRIEND_RESPOND: {
                    if (strcmp(sharedPdu->caData, ADD_FRIEND_EXITED) == 0) {
                        QMessageBox::warning(this, "添加好友", "添加失败: 好友已存在");
                    } else if (strcmp(sharedPdu->caData, ADD_FRIEND_OFFLINE) == 0) {
                        QMessageBox::warning(this, "添加好友", "添加失败: 用户不在线");
                    } else if (strcmp(sharedPdu->caData, ADD_FRIEND_NOEXIST) == 0) {
                        QMessageBox::warning(this, "添加好友", "添加失败: 用户不存在");
                    } else if (strcmp(sharedPdu->caData, ADD_FRIEND_SELF) == 0) {
                        QMessageBox::warning(this, "添加好友", "添加失败: 不能添加自己为好友");
                    } else if (strcmp(sharedPdu->caData, ADD_FRIEND_SENT) == 0) {
                        QMessageBox::information(this, "添加好友", "添加好友请求已发送");
                    }
                    break;
                }
                case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST: {
                    char perName[32];
                    memcpy(perName, sharedPdu->caData + 32, 32);
                    int ret = QMessageBox::information(this, "添加好友",
                                                       QString("%1 想添加你为好友，是否同意？").arg(perName),
                                                       QMessageBox::Yes, QMessageBox::No);
                    std::shared_ptr<PDU> rpdPDU = mk_shared_PDU(0);
                    if (ret == QMessageBox::Yes) {
                        rpdPDU->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_AGREE_RESPOND;
                    } else {
                        rpdPDU->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_REFUSE_RESPOND;
                    }
                    memcpy(rpdPDU->caData, sharedPdu->caData, 64);
                    m_tcpSocket.write((char*)rpdPDU.get(), rpdPDU->uiPDULen);
                    break;
                }
                case ENUM_MSG_TYPE_ADD_FRIEND_AGREE_RESPOND:
                case ENUM_MSG_TYPE_ADD_FRIEND_AGREE_RESPOND_RESPOND:
                case ENUM_MSG_TYPE_REFRESH_FRIEND_RESPOND: {
                    OpeWidget::getInstance().getFriendW()->refreshFriend(sharedPdu);
                    break;
                }
                case ENUM_MSG_TYPE_DELETE_FRIEND_RESPOND: {
                    if (strcmp(sharedPdu->caData, DELETE_FRIEND_OK) == 0) {
                        char delFriName[32];
                        memcpy(delFriName, sharedPdu->caData + 32, 32);
                        OpeWidget::getInstance().getFriendW()->delCurFriInfo(delFriName);
                        OpeWidget::getInstance().getFriendW()->refreshFriend(sharedPdu);
                        QMessageBox::information(this, "删除好友", "删除好友成功");
                    } else {
                        QMessageBox::warning(this, "删除好友", "删除好友失败");
                    }
                    break;
                }
                case ENUM_MSG_TYPE_SENDMSG_RESPOND: {
                    if (strcmp(sharedPdu->caData, SEND_MESSAGE_OK) == 0) {
                        OpeWidget::getInstance().getFriendW()->updateSendMsg();
                    } else if (strcmp(sharedPdu->caData, SEND_MESSAGE_OFFLINE) == 0) {
                        QMessageBox::warning(this, "发送消息", "发送失败: 好友不在线");
                    } else if (strcmp(sharedPdu->caData, SEND_MESSAGE_NOFRIEND) == 0) {
                        QMessageBox::warning(this, "发送消息", "发送失败: 不是好友");
                    } else if (strcmp(sharedPdu->caData, SEND_MESSAGE_NOEXIST) == 0) {
                        QMessageBox::warning(this, "发送消息", "发送失败: 用户不存在");
                    } else {
                        QMessageBox::warning(this, "发送消息", "发送失败: 未知错误");
                    }
                    break;
                }
                case ENUM_MSG_TYPE_SENDMSG_REQUEST:
                case ENUM_MSG_TYPE_SENDPUBLICMSG_REQUEST: {
                    OpeWidget::getInstance().getFriendW()->updateRecvMsg(sharedPdu);
                    break;
                }
                case ENUM_MSG_TYPE_CREATE_DIR_RESPOND:
                case ENUM_MSG_TYPE_CREATE_DIR_SAVE_RESPOND: {
                    if (strcmp(sharedPdu->caData, CREATE_DIR_OK) == 0) {
                        OpeWidget::getInstance().getWebDiskW()->updateFileInfo(sharedPdu);
                        QMessageBox::information(this, "新建文件夹", "新建文件夹成功");
                    } else if (strcmp(sharedPdu->caData, CREATE_DIR_PATH_NOEXIST) == 0) {
                        QMessageBox::warning(this, "新建文件夹", "新建文件夹失败: 路径不存在");
                    } else {
                        QMessageBox::warning(this, "新建文件夹", "新建文件夹失败: 文件已存在");
                    }
                    break;
                }
                case ENUM_MSG_TYPE_REFRESH_DIR_RESPOND:
                case ENUM_MSG_TYPE_REFRESH_SAVE_RESPOND:
                case ENUM_MSG_TYPE_DELETE_DIRORFILE_RESPOND:
                case ENUM_MSG_TYPE_RENAME_DIRORFILE_RESPOND: {
                    OpeWidget::getInstance().getWebDiskW()->updateFileInfo(sharedPdu);
                    break;
                }
                case ENUM_MSG_TYPE_ENTER_DIR_RESPOND:
                case ENUM_MSG_TYPE_ENTER_DIR_SAVE_RESPOND: {
                    if (strcmp(sharedPdu->caData, ENTER_DIR_OK) == 0) {
                        m_curPath = m_curPathTemp;
                        OpeWidget::getInstance().getWebDiskW()->updateFileInfo(sharedPdu);
                        FileSaveDialog::getInstance().setCurPath(m_curPath);
                        FileSaveDialog::getInstance().updateFileSaveDialog(sharedPdu);
                    } else {
                        QMessageBox::warning(this, "进入文件夹", "进入文件夹失败: 不是文件夹");
                    }
                    break;
                }
                case ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST_RESPOND: {
                    OpeWidget::getInstance().getWebDiskW()->uploadFileData();
                    break;
                }
                case ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND: {
                    OpeWidget::getInstance().getWebDiskW()->handleUploadFileResponse(sharedPdu);
                    break;
                }
                case ENUM_MSG_TYPE_SHARE_FILE_REQUEST: {
                    OpeWidget::getInstance().getFileRecvD()->addFileRecvDialog(sharedPdu);
                    break;
                }
                case ENUM_MSG_TYPE_SAVE_FILE_RESPOND: {
                    OpeWidget::getInstance().getWebDiskW()->handleSaveFileResponse(sharedPdu);
                    break;
                }
                case ENUM_MSG_TYPE_MOVE_FILE_RESPOND: {
                    OpeWidget::getInstance().getWebDiskW()->handleMoveFileResponse(sharedPdu);
                    break;
                }
                default:
                    qDebug() << "Unknown MsgType:" << sharedPdu->uiMsgType;
                    break;
                }
            } catch (const std::exception &e) {
                qDebug() << "recvMsg: Exception caught:" << e.what();
                QMessageBox::warning(this, "错误", "处理服务器响应时发生异常");
            }
        }
    } else {
        QByteArray buffer = m_tcpSocket.readAll();
        m_fileRecv.file.write(buffer);
        m_fileRecv.recvedSize += buffer.size();
        qint64 elapsedTime = m_elapsedTimer->elapsed();
        if (elapsedTime > 0) {
            double speed = (m_fileRecv.recvedSize / 1024.0) / (elapsedTime / 1000.0);
            QString process = QString("%1%").arg((double)m_fileRecv.recvedSize / m_fileRecv.totalSize * 100, 0, 'f', 2);
            OpeWidget::getInstance().getFileTransferW()->updateFTWItem(m_fileRecv.recvedSize, process,
                                                                       QString("%1 KB/s").arg(speed, 0, 'f', 2), "下载中");
        }
        if (m_fileRecv.recvedSize >= m_fileRecv.totalSize) {
            m_fileRecv.file.close();
            m_fileRecv.recvingFlag = false;
            OpeWidget::getInstance().getFileTransferW()->updateFTWItem(m_fileRecv.recvedSize, "100%", "0 KB/s", "下载完成");
            m_fileRecv.recvedSize = 0;
            m_fileRecv.totalSize = 0;
        }
    }
}
