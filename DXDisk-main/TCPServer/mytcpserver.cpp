#include "mytcpserver.h"
#include <QDebug>

// 构造函数
MyTcpServer::MyTcpServer(QObject *parent)
    : QTcpServer{parent}
{
}

// 单例模式
MyTcpServer &MyTcpServer::getInstance()
{
    static MyTcpServer instance;
    return instance;
}

// 获取套接字集合
QSet<MyTcpSocket*>& MyTcpServer::getTcpSocketSet()
{
    return m_tcpSocketSet;
}

// 处理新连接
void MyTcpServer::incomingConnection(qintptr socketDescriptor)
{
    qDebug() << "(MyTcpServer::incomingConnection) New client connected";
    MyTcpSocket *pTcpSocket = new MyTcpSocket(this);
    if (!pTcpSocket->setSocketDescriptor(socketDescriptor)) {
        qDebug() << "Failed to set socket descriptor";
        delete pTcpSocket;
        return;
    }
    m_tcpSocketSet.insert(pTcpSocket);
    connect(pTcpSocket, &MyTcpSocket::offline, this, &MyTcpServer::deleteSocket);
}

// 转发消息
void MyTcpServer::transpond(QString name, std::shared_ptr<PDU> pdu)
{
    if (name.isEmpty() || !pdu) {
        qDebug() << "transpond: Invalid name or PDU";
        return;
    }

    for (MyTcpSocket *socket : m_tcpSocketSet) {
        if (socket && socket->getName() == name) {
            socket->write((char*)pdu.get(), pdu->uiPDULen);
            break;
        }
    }
}

// 删除下线客户端
void MyTcpServer::deleteSocket(MyTcpSocket *mysocket)
{
    if (!mysocket) {
        qDebug() << "deleteSocket: Null socket";
        return;
    }

    auto iter = m_tcpSocketSet.find(mysocket);
    if (iter != m_tcpSocketSet.end()) {
        (*iter)->deleteLater();
        m_tcpSocketSet.erase(iter);
        qDebug() << "Client disconnected:" << mysocket->getName();
    }
}
