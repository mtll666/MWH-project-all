#include "filetransferwidget.h"
#include "ui_filetransferwidget.h"
#include "UniversalFunction.h"
#include "opeWidget.h"

FileTransferWidget::FileTransferWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FileTransferWidget)
{
    ui->setupUi(this);

    ui->m_pFileTransferTreeW->setColumnWidth(0,160);    //文件名
    ui->m_pFileTransferTreeW->setColumnWidth(1,90);     //文件类型
    ui->m_pFileTransferTreeW->setColumnWidth(2,75);     //文件已下载大小
    ui->m_pFileTransferTreeW->setColumnWidth(3,75);     //文件大小
    ui->m_pFileTransferTreeW->setColumnWidth(4,75);     //下载进度
    ui->m_pFileTransferTreeW->setColumnWidth(5,75);     //下载速度
    ui->m_pFileTransferTreeW->setColumnWidth(6,85);     //下载状态

    // 设置第一列宽度可拉伸
    ui->m_pFileTransferTreeW->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    // 设置宽度固定
    ui->m_pFileTransferTreeW->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    ui->m_pFileTransferTreeW->header()->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->m_pFileTransferTreeW->header()->setSectionResizeMode(3, QHeaderView::Fixed);
    ui->m_pFileTransferTreeW->header()->setSectionResizeMode(4, QHeaderView::Fixed);
    ui->m_pFileTransferTreeW->header()->setSectionResizeMode(5, QHeaderView::Fixed);
    ui->m_pFileTransferTreeW->header()->setSectionResizeMode(6, QHeaderView::Fixed);
}

FileTransferWidget::~FileTransferWidget()
{
    delete ui;
}

QTreeWidget* FileTransferWidget::getWidget()
{
    return ui->m_pFileTransferTreeW;
}

void FileTransferWidget::createFTWItem(QString downloadFileName, int fileType,
                                       qint64 fileSize, QString status)
{
    QTreeWidgetItem *pFileTransferItem =new QTreeWidgetItem(ui->m_pFileTransferTreeW);
    //显示文件名字
    pFileTransferItem->setText(0,downloadFileName);
    //显示icon,文件类型
    setFileTypeIcon(pFileTransferItem,fileType,0,1);
    //显示已下载大小
    pFileTransferItem->setText(2,"0");
    //显示文件大小
    pFileTransferItem->setText(3,byteConversion(fileSize));
    //显示下载进度
    pFileTransferItem->setText(4,"0%");
    //显示下载速度
    pFileTransferItem->setText(5,"0");
    //显示下载状态
    pFileTransferItem->setText(6,status);
}

void FileTransferWidget::updateFTWItem(qint64 recvedSize, QString process,
                                       QString speed, QString status)
{
    auto size = OpeWidget::getInstance().getFileTransferW()->getWidget()->topLevelItemCount();
    if(size == 0)return;
    QTreeWidgetItem *pFileTransferItem =
        OpeWidget::getInstance().getFileTransferW()->getWidget()->topLevelItem(size - 1);

    //更新已下载大小
    pFileTransferItem->setText(2,byteConversion(recvedSize));
    //更新下载进度
    pFileTransferItem->setText(4,process);
    //更新下载速度
    pFileTransferItem->setText(5,speed);
    //更新下载状态
    pFileTransferItem->setText(6,status);
}

