#include "filerecvdialog.h"
#include "UniversalFunction.h"
#include "ui_filerecvdialog.h"
#include "opewidget.h"
#include "filesavedialog.h"

FileRecvDialog::FileRecvDialog(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FileRecvDialog)
{
    ui->setupUi(this);

    ui->fileInfoTW->setColumnWidth(0,240);
    ui->fileInfoTW->setColumnWidth(1,90);
    ui->fileInfoTW->setColumnWidth(2,80);
    ui->fileInfoTW->setColumnWidth(3,180);

    // 设置第一列宽度可拉伸
    ui->fileInfoTW->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    // 设置第二、三列宽度固定
    ui->fileInfoTW->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    ui->fileInfoTW->header()->setSectionResizeMode(2, QHeaderView::Fixed);
    // 设置第四列宽度可拉伸
    ui->fileInfoTW->header()->setSectionResizeMode(3, QHeaderView::Stretch);

    isEmpty();
}

FileRecvDialog::~FileRecvDialog()
{
    delete ui;
}

//判断列表中是否还有未处理的消息
void FileRecvDialog::isEmpty()
{
    if(ui->fileInfoTW->topLevelItemCount() == 0)
    {
        QTreeWidgetItem *pFileInfoItem = new QTreeWidgetItem(ui->fileInfoTW);
        pFileInfoItem->setText(0,"暂无好友向你分享文件");
        ui->fileInfoTW->setEnabled(false);
    }
}

void FileRecvDialog::addFileRecvDialog(std::shared_ptr<PDU> pdu)
{
    if(pdu == nullptr) return;
    if(!ui->fileInfoTW->isEnabled())
    {
        ui->fileInfoTW->clear();
        ui->fileInfoTW->setEnabled(true);
    }

    if(OpeWidget::getInstance().getListW()->currentRow() != 3)
    {
        OpeWidget::getInstance().getListW()->item(3)
            ->setIcon(QIcon(QPixmap(":/icon/home_item_icon/recvRD.png")));
    }
    FileInfo *pFileInfo = (FileInfo*)(pdu->caMsg);
    QTreeWidgetItem *pFileInfoItem = new QTreeWidgetItem(ui->fileInfoTW);
    //显示文件名字,icon,文件类型
    pFileInfoItem->setText(0,pFileInfo->caFileNameUtf8);
    qDebug() << "(FileRecvDialog::addFileRecvDialog)分享文件名称:" << pFileInfo->caFileNameUtf8;
    setFileTypeIcon(pFileInfoItem,pFileInfo->iFileType,0,1);
    //显示文件大小
    pFileInfoItem->setText(2,byteConversion(pFileInfo->llFileSize));
    qDebug() << "(FileRecvDialog::addFileRecvDialog)分享文件大小:" << pFileInfo->llFileSize;
    //显示发送方名字
    pFileInfoItem->setText(3,pdu->caData);
    qDebug() << "(FileRecvDialog::addFileRecvDialog)分享者:" << pdu->caData;

    QString shareFilePath = pdu->caMsg + sizeof(FileInfo);
    qDebug() << "(FileRecvDialog::addFileRecvDialog)分享文件路径:" << shareFilePath;
    shareFilePathList.append(shareFilePath);
}

QStringList &FileRecvDialog::getShareFilePathList()
{
    return shareFilePathList;
}

int FileRecvDialog::getSaveFileIndex()
{
    return saveFileIndex;
}

void FileRecvDialog::deleteFileInfoItem(int index)
{
    ui->fileInfoTW->takeTopLevelItem(index);
    isEmpty();
}

void FileRecvDialog::releaseAgreePB()
{
    ui->agreePB->setEnabled(true);     //在处理完文件请求时释放按键
    ui->refusePB->setEnabled(true);     //在处理完文件请求时释放按键
}

void FileRecvDialog::on_agreePB_clicked()
{
    QTreeWidgetItem *pItem = ui->fileInfoTW->currentItem();
    if(pItem != nullptr)
    {
        saveFileIndex = ui->fileInfoTW->indexOfTopLevelItem(pItem);
        FileSaveDialog::getInstance().refreshDialog();
        FileSaveDialog::getInstance().show();
        ui->agreePB->setEnabled(false);     //在处理一个文件请求时将按键置为不可再点击
        ui->refusePB->setEnabled(false);     //在处理一个文件请求时将按键置为不可再点击
    }
}

void FileRecvDialog::on_refusePB_clicked()
{
    QTreeWidgetItem *pItem = ui->fileInfoTW->currentItem();
    if(pItem != nullptr)
    {
        shareFilePathList.removeAt(ui->fileInfoTW->indexOfTopLevelItem(pItem));
        deleteFileInfoItem(ui->fileInfoTW->indexOfTopLevelItem(pItem));
    }
}



