#include "tcpclient.h"
#include <QApplication>
#include "sharefilewidget.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    TCPClient::getInstance().show();

    return a.exec();
}
