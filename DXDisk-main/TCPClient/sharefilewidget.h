#ifndef SHAREFILEWIDGET_H
#define SHAREFILEWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QButtonGroup>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCheckBox>

class ShareFileWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ShareFileWidget(QWidget *parent = nullptr);
    static ShareFileWidget &getInstance();
    void updateShareFriList(QStringList *onlineFriList);

private slots:
    void on_refreshPB_clicked();
    void on_allSelectPB_clicked();
    void on_cancelSelectPB_clicked();
    void on_okPB_clicked();
    void on_cancelPB_clicked();

private:
    QPushButton *m_pAllSelectPB;
    QPushButton *m_pCancelSelectPB;
    QPushButton *m_pRefreshPB;
    QPushButton *m_pOkPB;
    QPushButton *m_pCancelPB;

    QScrollArea *m_pSA;
    QWidget *m_pOnlineFriendW;
    QVBoxLayout *m_pOnlineFriendVBL;
    QButtonGroup *m_pButtonGroup;
};

#endif // SHAREFILEWIDGET_H

