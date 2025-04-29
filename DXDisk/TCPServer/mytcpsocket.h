#ifndef MYTCPSOCKET_H
#define MYTCPSOCKET_H

#include <QTcpSocket>
#include "protocol.h"
#include <QDir>
#include <QFile>

// 自定义TCP套接字类，处理客户端连接
class MyTcpSocket : public QTcpSocket
{
    Q_OBJECT
public:
    explicit MyTcpSocket(QObject *parent = nullptr);
    std::shared_ptr<PDU> refreshDirPDU(QString pCurPath); // 生成刷新文件夹PDU
    void handleRqsPDU(std::shared_ptr<PDU> pdu); // 处理客户端请求
    QString getName(); // 获取用户名
    bool copyDir(QString srcDir, QString destDir); // 拷贝文件夹

signals:
    void offline(MyTcpSocket *mysocket); // 客户端下线信号

public slots:
    void recvMsg(); // 接收消息
    void clientOffline(); // 处理客户端下线

private:
    QString socketName; // 登录用户名
    FileRecv fileRecv; // 文件接收对象
    QString m_curPath; // 当前上传文件路径
};

#endif // MYTCPSOCKET_H
