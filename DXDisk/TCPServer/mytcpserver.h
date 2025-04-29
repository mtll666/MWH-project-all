#ifndef MYTCPSERVER_H
#define MYTCPSERVER_H

#include <QTcpServer>
#include <QSet>
#include "mytcpsocket.h"

// 自定义TCP服务器类
class MyTcpServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit MyTcpServer(QObject *parent = nullptr);
    static MyTcpServer& getInstance(); // 单例模式
    QSet<MyTcpSocket*>& getTcpSocketSet(); // 获取套接字集合
    void incomingConnection(qintptr socketDescriptor) override; // 处理新连接
    void transpond(QString name, std::shared_ptr<PDU> pdu); // 转发消息

public slots:
    void deleteSocket(MyTcpSocket *mysocket); // 删除下线客户端

private:
    QSet<MyTcpSocket*> m_tcpSocketSet; // 客户端套接字集合
};

#endif // MYTCPSERVER_H
