#ifndef MESSAGEWIDGET_H
#define MESSAGEWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QVBoxLayout>

class MessageWidget : public QWidget
{
    Q_OBJECT
    friend class FriendWidget;
public:
    explicit MessageWidget(QWidget *parent = nullptr);
    void setContent(const QString &content);

signals:

private:
    QTextEdit *perNameTE;
    QTextEdit *contentTE;
};

#endif // MESSAGEWIDGET_H
