#include "filesavedialog.h"
#include "ui_filesavedialog.h"
#include "UniversalFunction.h"
#include "tcpclient.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QCloseEvent>

FileSaveDialog::FileSaveDialog(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FileSaveDialog)
{
    ui->setupUi(this);

    ui->FileInfoTreeW->setColumnWidth(0,240);
    ui->FileInfoTreeW->setColumnWidth(1,90);
    ui->FileInfoTreeW->setColumnWidth(2,80);

    // 设置第一列宽度可拉伸
    ui->FileInfoTreeW->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    // 设置第二、三列宽度固定
    ui->FileInfoTreeW->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    ui->FileInfoTreeW->header()->setSectionResizeMode(2, QHeaderView::Fixed);
}

FileSaveDialog::~FileSaveDialog()
{
    delete ui;
}

FileSaveDialog &FileSaveDialog::getInstance()
{
    static FileSaveDialog instance;
    return instance;
}

void FileSaveDialog::setCurPath(QString path)
{
    curPath = path;
}

QString FileSaveDialog::getCurPathTemp()
{
    return curPathTemp;
}

void FileSaveDialog::refreshDialog()
{
    std::shared_ptr<PDU> pdu = mk_shared_PDU(curPath.toUtf8().size() + 1);
    pdu->uiMsgType = ENUM_MSG_TYPE_REFRESH_SAVE_REQUEST;
    memcpy(pdu->caMsg,curPath.toUtf8(),curPath.toUtf8().size());
    TCPClient::getInstance().getTCPSocket().write((char*)pdu.get(),pdu->uiPDULen);
}


void FileSaveDialog::updateFileSaveDialog(std::shared_ptr<PDU> pdu)
{
    if(pdu == nullptr) return;
    ui->FileInfoTreeW->clear();
    FileInfo *pFileInfo = nullptr;
    int iCount = pdu->uiMsgLen/sizeof(FileInfo);
    if(iCount == 0)
    {
        QTreeWidgetItem *pFileInfoItem = new QTreeWidgetItem(ui->FileInfoTreeW);
        pFileInfoItem->setText(0,"此文件夹为空");
        ui->FileInfoTreeW->setEnabled(false);
    }
    else
    {
        ui->FileInfoTreeW->setEnabled(true);
        for(int i = 0; i < iCount; ++i)
        {
            pFileInfo = (FileInfo*)pdu->caMsg + i;
            QTreeWidgetItem *pFileInfoItem = new QTreeWidgetItem(ui->FileInfoTreeW);
            //显示文件名字,icon,文件类型
            pFileInfoItem->setText(0,pFileInfo->caFileNameUtf8);
            setFileTypeIcon(pFileInfoItem,pFileInfo->iFileType,0,1);
            //显示文件大小
            pFileInfoItem->setText(2,byteConversion(pFileInfo->llFileSize));

            QPair<int,qint64> fileInfoPair;
            fileInfoPair.first = pFileInfo->iFileType;
            fileInfoPair.second = pFileInfo->llFileSize;
        }
    }
}

void FileSaveDialog::setSaveFlag(bool ret)
{
    isSaveFlag = ret;
}

void FileSaveDialog::closeEvent(QCloseEvent *event)
{
    OpeWidget::getInstance().getFileRecvD()->releaseAgreePB();

    // 允许窗口关闭
    event->accept();
}



void FileSaveDialog::on_returnPB_clicked()
{
    QString rootPath = TCPClient::getInstance().getRootPath();
    if(curPath == rootPath)
        QMessageBox::warning(this,"返回","返回失败:根目录下无法再返回!");
    else
    {
        int index = curPath.lastIndexOf('/');
        curPath.remove(index,curPath.size()-index); //得到上一级路径
        refreshDialog();
    }
}


void FileSaveDialog::on_FileInfoTreeW_doubleClicked(const QModelIndex &index)
{
    QString dirName = index.data().toString();
    curPathTemp = curPath + "/" + dirName;

    std::shared_ptr<PDU> pdu = mk_shared_PDU(curPath.toUtf8().size()+1);
    pdu->uiMsgType = ENUM_MSG_TYPE_ENTER_DIR_SAVE_REQUEST;
    strncpy(pdu->caData,dirName.toUtf8(),dirName.toUtf8().size());
    memcpy(pdu->caMsg,curPath.toUtf8(),curPath.toUtf8().size());
    TCPClient::getInstance().getTCPSocket().write((char*)pdu.get(),pdu->uiPDULen);
}


void FileSaveDialog::on_createDirPB_clicked()
{
    bool isOk;
    QString strNewDir = QInputDialog::getText(this,"新建文件夹","输入名字(不超过63个字符,中文占3个字符):",
                                              QLineEdit::Normal,"",&isOk);

    if(!strNewDir.isEmpty())
    {
        if(strNewDir.toUtf8().size()>63)QMessageBox::warning(this,"新建文件夹","输入名字超过63个字符!");
        else
        {
            std::shared_ptr<PDU> pdu = mk_shared_PDU(curPath.toUtf8().size()+1);
            pdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_SAVE_REQUEST;
            strncpy(pdu->caData,strNewDir.toUtf8(),strNewDir.toUtf8().size());
            memcpy(pdu->caMsg,curPath.toUtf8(),curPath.toUtf8().size());
            TCPClient::getInstance().getTCPSocket().write((char*)pdu.get(),pdu->uiPDULen);
        }
    }
    else if(isOk) QMessageBox::warning(this,"新建文件夹","输入名字不能为空!");
}


void FileSaveDialog::on_savePB_clicked()
{
    //保存好友分享的文件
    if(isSaveFlag)
    {
        int saveFileIndex = OpeWidget::getInstance().getFileRecvD()->getSaveFileIndex();
        QString shareFilePath = OpeWidget::getInstance()
                                    .getFileRecvD()->getShareFilePathList().at(saveFileIndex);
        std::shared_ptr<PDU> pdu = mk_shared_PDU(curPath.toUtf8().size() + 1
                                                 + shareFilePath.toUtf8().size() + 1);
        pdu->uiMsgType = ENUM_MSG_TYPE_SAVE_FILE_REQUEST;
        memcpy(pdu->caMsg,curPath.toUtf8(),curPath.toUtf8().size());
        memcpy(pdu->caMsg + curPath.toUtf8().size() + 1,
               shareFilePath.toUtf8(),
               shareFilePath.toUtf8().size());
        TCPClient::getInstance().getTCPSocket().write((char*)pdu.get(),pdu->uiPDULen);
    }
    //移动文件
    else
    {
        QString moveFilePath = OpeWidget::getInstance().getWebDiskW()->getMoveFilePath();
        std::shared_ptr<PDU> pdu = mk_shared_PDU(curPath.toUtf8().size() + 1
                                                 + moveFilePath.toUtf8().size() + 1);
        pdu->uiMsgType = ENUM_MSG_TYPE_MOVE_FILE_REQUEST;
        memcpy(pdu->caMsg,curPath.toUtf8(),curPath.toUtf8().size());
        memcpy(pdu->caMsg + curPath.toUtf8().size() + 1,
               moveFilePath.toUtf8(),
               moveFilePath.toUtf8().size());
        TCPClient::getInstance().getTCPSocket().write((char*)pdu.get(),pdu->uiPDULen);
    }
}


void FileSaveDialog::on_cancelPB_clicked()
{
    this->close();
}

