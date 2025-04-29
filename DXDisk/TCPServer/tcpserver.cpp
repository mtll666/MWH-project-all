#include "tcpserver.h"
#include "ui_tcpserver.h"
#include <QByteArray>
#include <QDebug>
#include <QMessageBox>
#include <QHostAddress>
#include <QFile>
#include <QRegularExpression> // 添加 QRegularExpression 头文件以解决 incomplete type 错误

TCPServer::TCPServer(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TCPServer)
{
    ui->setupUi(this);

    loadConfig();
    MyTcpServer::getInstance().listen(QHostAddress::Any, m_usPort); // 监听
}

TCPServer::~TCPServer()
{
    delete ui;
}

void TCPServer::loadConfig()
{
    QFile file(":/server.config"); // 打开资源文件
    if(file.open(QIODeviceBase::ReadOnly))
    {
        QByteArray baData = file.readAll();
        QString strData = QString::fromUtf8(baData).trimmed(); // 转换为字符串并去除首尾空白
        file.close();

        // 支持多种换行符（\n, \r\n, \r），并跳过空行
        QStringList strList = strData.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
        qDebug() << "(TCPServer::loadConfig)读取到的配置内容:" << strList;

        if(strList.size() < 2)
        {
            QMessageBox::critical(this, "打开配置", "server.config 格式错误，缺少IP或端口");
            qDebug() << "(TCPServer::loadConfig)配置错误: 文件内容不足，行数:" << strList.size();
            return;
        }

        m_strIP = strList[0].trimmed();
        bool ok;
        m_usPort = strList[1].trimmed().toUShort(&ok);
        if(!ok)
        {
            QMessageBox::critical(this, "打开配置", "server.config 端口格式错误");
            qDebug() << "(TCPServer::loadConfig)配置错误: 端口无效，输入:" << strList[1];
            return;
        }

        qDebug() << "(TCPServer::loadConfig)服务器配置成功: ip:" << m_strIP << " port:" << m_usPort;
    }
    else
    {
        QMessageBox::critical(this, "打开配置", "无法打开 server.config 文件");
        qDebug() << "(TCPServer::loadConfig)打开配置文件失败";
    }
}

void TCPServer::disconnectAllClients()
{
    qDebug() << "(TCPServer::disconnectAllClients)断开所有客户端连接...";

    for(QTcpSocket *socket : MyTcpServer::getInstance().getTcpSocketSet())
    {
        socket->disconnectFromHost();
    }

    MyTcpServer::getInstance().getTcpSocketSet().clear();

    // 关闭服务器以停止监听新连接
    close();
}
