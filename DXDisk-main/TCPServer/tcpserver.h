#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QWidget>
#include "mytcpserver.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class TCPServer;
}
QT_END_NAMESPACE

class TCPServer : public QWidget
{
    Q_OBJECT

public:
    TCPServer(QWidget *parent = nullptr);
    ~TCPServer();

    void loadConfig();

public slots:
    void disconnectAllClients();    //服务器下线，断开与所有客户端的连接

private:
    Ui::TCPServer *ui;
    QString m_strIP;    //存放IP
    quint16 m_usPort;   //存放端口
};
#endif // TCPSERVER_H
