#include "tcpserver.h"
#include "opedb.h"
#include <QApplication>

// 服务端主函数
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    OpeDB::getInstance().init(); // 初始化数据库

    TCPServer w;
    w.show();

    QObject::connect(&a, &QCoreApplication::aboutToQuit,
                     &w, &TCPServer::disconnectAllClients);

    return a.exec();
}
