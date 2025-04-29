#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QWidget>
#include <QFile>
#include <QTcpSocket>
#include <QElapsedTimer>
#include "protocol.h"
#include "opewidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class TCPClient;
}
QT_END_NAMESPACE

class TCPClient : public QWidget
{
    Q_OBJECT

public:
    explicit TCPClient(QWidget *parent = nullptr);
    ~TCPClient();
    static TCPClient &getInstance();
    void loadConfig();
    QTcpSocket &getTCPSocket();
    QString getLoginName();
    QString getRootPath();
    QString getCurPath();
    void setCurPathTemp(QString strCurPathTemp);
    FileRecv &getFileRecv();
    void setFileRecv();
    void startElapsedTimer();

public slots:
    void showConnect();
    void clientOffline();
    void on_connect_pb_clicked();
    void on_regist_pb_clicked();
    void on_login_pb_clicked();
    void recvMsg();

private:
    Ui::TCPClient *ui;
    QString m_strIP;
    quint16 m_usPort;
    QTcpSocket m_tcpSocket;
    QString m_loginName; // 登录用户名
    QString m_rootPath;  // 根路径
    QString m_curPath;   // 当前路径
    QString m_curPathTemp; // 当前路径临时值
    FileRecv m_fileRecv; // 文件接收对象
    QElapsedTimer *m_elapsedTimer; // 计时器
};

#endif // TCPCLIENT_H
