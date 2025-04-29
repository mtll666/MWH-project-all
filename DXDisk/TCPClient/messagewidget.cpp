#include "messagewidget.h"

MessageWidget::MessageWidget(QWidget *parent)
    : QWidget{parent}
{
    perNameTE = new QTextEdit;
    perNameTE->setReadOnly(true);
    perNameTE->setFixedHeight(30);
    perNameTE->setAlignment(Qt::AlignHCenter);
    perNameTE->adjustSize();
    contentTE = new QTextEdit;
    contentTE->setReadOnly(true);

    QVBoxLayout *VBL = new QVBoxLayout;
    VBL->addWidget(perNameTE);
    VBL->addWidget(contentTE);
    setLayout(VBL);
}

void MessageWidget::setContent(const QString &content)
{
    contentTE->append(content);
}
