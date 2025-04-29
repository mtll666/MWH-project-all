#ifndef ONLINEUSR_H
#define ONLINEUSR_H

#include <QWidget>
#include "protocol.h"

namespace Ui {
class OnlineUsr;
}

class OnlineUsr : public QWidget
{
    Q_OBJECT

public:
    explicit OnlineUsr(QWidget *parent = nullptr);
    ~OnlineUsr();
    void showUsr(std::shared_ptr<PDU> pdu);

private slots:
    void on_addFriend_pb_clicked();

private:
    Ui::OnlineUsr *ui;
};

#endif // ONLINEUSR_H
