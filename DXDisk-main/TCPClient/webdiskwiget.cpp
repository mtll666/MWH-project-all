#include "webdiskwiget.h"
#include "ui_webdiskwiget.h"
#include "tcpclient.h"
#include "UniversalFunction.h"
#include "filesavedialog.h"
#include "sharefilewidget.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>

// 构造函数，初始化 UI 和信号槽
WebDiskWiget::WebDiskWiget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::WebDiskWiget)
{
    ui->setupUi(this);
    ui->m_pFileInfoTreeW->setColumnWidth(0, 240);
    ui->m_pFileInfoTreeW->setColumnWidth(1, 90);
    ui->m_pFileInfoTreeW->setColumnWidth(2, 80);
    ui->m_pFileInfoTreeW->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->m_pFileInfoTreeW->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    ui->m_pFileInfoTreeW->header()->setSectionResizeMode(2, QHeaderView::Fixed);
}

// 析构函数，清理 UI
WebDiskWiget::~WebDiskWiget()
{
    delete ui;
}

// 更新文件信息
void WebDiskWiget::updateFileInfo(std::shared_ptr<PDU> pdu)
{
    if (!pdu) return;
    ui->m_pFileInfoTreeW->clear();
    int iCount = pdu->uiMsgLen / sizeof(FileInfo);
    if (iCount == 0) {
        QTreeWidgetItem *pFileInfoItem = new QTreeWidgetItem(ui->m_pFileInfoTreeW);
        pFileInfoItem->setText(0, "此文件夹为空");
        ui->m_pFileInfoTreeW->setEnabled(false);
    } else {
        ui->m_pFileInfoTreeW->setEnabled(true);
        for (int i = 0; i < iCount; ++i) {
            FileInfo *pFileInfo = (FileInfo*)pdu->caMsg + i;
            QTreeWidgetItem *pFileInfoItem = new QTreeWidgetItem(ui->m_pFileInfoTreeW);
            pFileInfoItem->setText(0, pFileInfo->caFileNameUtf8);
            setFileTypeIcon(pFileInfoItem, pFileInfo->iFileType, 0, 1);
            pFileInfoItem->setText(2, byteConversion(pFileInfo->llFileSize));
        }
    }
}

// 发送上传文件数据
void WebDiskWiget::uploadFileData()
{
    if (!m_uploadFile.isOpen()) {
        qDebug() << "uploadFileData: File not open";
        return;
    }
    char buffer[4096];
    qint64 ret;
    while ((ret = m_uploadFile.read(buffer, 4096)) > 0) {
        TCPClient::getInstance().getTCPSocket().write(buffer, ret);
        TCPClient::getInstance().getTCPSocket().waitForBytesWritten(1000);
    }
    if (ret < 0) {
        qDebug() << "uploadFileData: Failed to read upload file";
        QMessageBox::warning(this, "上传文件", "读取文件失败");
    }
}

// 处理上传文件回复
void WebDiskWiget::handleUploadFileResponse(std::shared_ptr<PDU> pdu)
{
    if (!pdu) return;
    if (strcmp(pdu->caData, UPLOAD_FILE_OK) == 0) {
        QMessageBox::information(this, "上传文件", "上传文件成功");
        OpeWidget::getInstance().getFileTransferW()->updateFTWItem(m_uploadFile.size(), "100%", "0 KB/s", "上传完成");
    } else {
        QMessageBox::warning(this, "上传文件", "上传文件失败");
        OpeWidget::getInstance().getFileTransferW()->updateFTWItem(0, "0%", "0 KB/s", "上传失败");
    }
    if (m_uploadFile.isOpen()) {
        m_uploadFile.close();
    }
}

// 处理保存文件回复
void WebDiskWiget::handleSaveFileResponse(std::shared_ptr<PDU> pdu)
{
    if (!pdu) return;
    if (strcmp(pdu->caData, SAVE_FILE_OK) == 0) {
        int saveFileIndex = OpeWidget::getInstance().getFileRecvD()->getSaveFileIndex();
        OpeWidget::getInstance().getFileRecvD()->deleteFileInfoItem(saveFileIndex);
        OpeWidget::getInstance().getFileRecvD()->getShareFilePathList().removeAt(saveFileIndex);
        FileSaveDialog::getInstance().close();
        QMessageBox::information(this, "保存文件", "保存文件成功");
    } else if (strcmp(pdu->caData, SAVE_FILE_FAILED_EXISTED) == 0) {
        QMessageBox::warning(this, "保存文件", "保存文件失败: 文件已存在");
    } else {
        QMessageBox::warning(this, "保存文件", "保存文件失败");
    }
    OpeWidget::getInstance().getFileRecvD()->releaseAgreePB();
}

// 处理移动文件回复
void WebDiskWiget::handleMoveFileResponse(std::shared_ptr<PDU> pdu)
{
    if (!pdu) return;
    if (strcmp(pdu->caData, MOVE_FILE_OK) == 0) {
        FileSaveDialog::getInstance().close();
        refreshDir();
        QMessageBox::information(this, "移动文件", "移动文件成功");
    } else {
        QMessageBox::warning(this, "移动文件", "移动文件失败");
    }
}

// 获取分享文件名
QString WebDiskWiget::getShareFileName()
{
    return m_shareFileName;
}

// 获取移动文件路径
QString WebDiskWiget::getMoveFilePath()
{
    return m_moveFilePath;
}

// 刷新文件夹
void WebDiskWiget::refreshDir()
{
    QString curPath = TCPClient::getInstance().getCurPath();
    if (curPath.isEmpty()) {
        qDebug() << "refreshDir: Current path is empty";
        return;
    }
    std::shared_ptr<PDU> pdu = mk_shared_PDU(curPath.toUtf8().size() + 1);
    pdu->uiMsgType = ENUM_MSG_TYPE_REFRESH_DIR_REQUEST;
    memcpy(pdu->caMsg, curPath.toUtf8(), curPath.toUtf8().size());
    TCPClient::getInstance().getTCPSocket().write((char*)pdu.get(), pdu->uiPDULen);
}

// 返回按钮
void WebDiskWiget::on_m_pReturnPB_clicked()
{
    QString rootPath = TCPClient::getInstance().getRootPath();
    QString curPath = TCPClient::getInstance().getCurPath();
    if (curPath == rootPath) {
        QMessageBox::warning(this, "返回", "返回失败: 已处于根目录");
    } else {
        int index = curPath.lastIndexOf('/');
        QString parentPath = curPath.left(index);
        TCPClient::getInstance().setCurPathTemp(parentPath);
        std::shared_ptr<PDU> pdu = mk_shared_PDU(curPath.toUtf8().size() + 1);
        pdu->uiMsgType = ENUM_MSG_TYPE_ENTER_DIR_REQUEST;
        memcpy(pdu->caMsg, curPath.toUtf8(), curPath.toUtf8().size());
        TCPClient::getInstance().getTCPSocket().write((char*)pdu.get(), pdu->uiPDULen);
    }
}

// 新建文件夹
void WebDiskWiget::on_m_pCreateDirPB_clicked()
{
    bool isOk;
    QString strNewDir = QInputDialog::getText(this, "新建文件夹", "输入名字(不超过63个字符):",
                                              QLineEdit::Normal, "", &isOk);
    if (!strNewDir.isEmpty() && isOk) {
        if (strNewDir.toUtf8().size() > 63) {
            QMessageBox::warning(this, "新建文件夹", "输入名字超过63个字符!");
        } else {
            QString curPath = TCPClient::getInstance().getCurPath();
            std::shared_ptr<PDU> pdu = mk_shared_PDU(curPath.toUtf8().size() + 1);
            pdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_REQUEST;
            strncpy(pdu->caData, strNewDir.toUtf8(), strNewDir.toUtf8().size());
            memcpy(pdu->caMsg, curPath.toUtf8(), curPath.toUtf8().size());
            TCPClient::getInstance().getTCPSocket().write((char*)pdu.get(), pdu->uiPDULen);
        }
    } else if (isOk) {
        QMessageBox::warning(this, "新建文件夹", "输入名字不能为空!");
    }
}

// 删除文件
void WebDiskWiget::on_m_pDeletePB_clicked()
{
    QTreeWidgetItem *item = ui->m_pFileInfoTreeW->currentItem();
    if (!item) {
        QMessageBox::warning(this, "删除文件", "请先选择文件或文件夹");
        return;
    }
    QString curPath = TCPClient::getInstance().getCurPath();
    std::shared_ptr<PDU> pdu = mk_shared_PDU(curPath.toUtf8().size() + 1);
    pdu->uiMsgType = ENUM_MSG_TYPE_DELETE_DIRORFILE_REQUEST;
    strncpy(pdu->caData, item->text(0).toUtf8(), 64);
    memcpy(pdu->caMsg, curPath.toUtf8(), curPath.toUtf8().size());
    TCPClient::getInstance().getTCPSocket().write((char*)pdu.get(), pdu->uiPDULen);
}

// 刷新文件夹
void WebDiskWiget::on_m_pRefreshDirPB_clicked()
{
    refreshDir();
}

// 重命名文件
void WebDiskWiget::on_m_pRenameFilePB_clicked()
{
    QTreeWidgetItem *item = ui->m_pFileInfoTreeW->currentItem();
    if (!item) {
        QMessageBox::warning(this, "重命名文件", "请先选择文件或文件夹");
        return;
    }
    bool isOk;
    QString newName = QInputDialog::getText(this, "重命名", "输入新名字(不超过63个字符):",
                                            QLineEdit::Normal, item->text(0), &isOk);
    if (!newName.isEmpty() && isOk) {
        if (newName.toUtf8().size() > 63) {
            QMessageBox::warning(this, "重命名文件", "输入名字超过63个字符!");
        } else {
            QString curPath = TCPClient::getInstance().getCurPath();
            QString oldName = item->text(0);
            std::shared_ptr<PDU> pdu = mk_shared_PDU(oldName.toUtf8().size() + 1 + curPath.toUtf8().size() + 1);
            pdu->uiMsgType = ENUM_MSG_TYPE_RENAME_DIRORFILE_REQUEST;
            strncpy(pdu->caData, newName.toUtf8(), 64);
            memcpy(pdu->caMsg, oldName.toUtf8(), oldName.toUtf8().size());
            memcpy(pdu->caMsg + oldName.toUtf8().size() + 1, curPath.toUtf8(), curPath.toUtf8().size());
            TCPClient::getInstance().getTCPSocket().write((char*)pdu.get(), pdu->uiPDULen);
        }
    } else if (isOk) {
        QMessageBox::warning(this, "重命名文件", "输入名字不能为空!");
    }
}

// 上传文件
void WebDiskWiget::on_m_pUploadFilePB_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择文件");
    if (filePath.isEmpty()) return;
    QFileInfo fileInfo(filePath);
    qint64 fileSize = fileInfo.size();
    QString fileName = fileInfo.fileName();
    QString curPath = TCPClient::getInstance().getCurPath();

    if (m_uploadFile.isOpen()) {
        m_uploadFile.close();
    }
    m_uploadFile.setFileName(filePath);
    if (!m_uploadFile.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "上传文件", "无法打开文件");
        return;
    }

    QString msgContent = QString("%1 %2").arg(fileSize).arg(curPath);
    std::shared_ptr<PDU> pdu = mk_shared_PDU(msgContent.toUtf8().size() + 1);
    pdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST;
    strncpy(pdu->caData, fileName.toUtf8(), 64);
    memcpy(pdu->caMsg, msgContent.toUtf8(), msgContent.toUtf8().size());
    TCPClient::getInstance().getTCPSocket().write((char*)pdu.get(), pdu->uiPDULen);

    OpeWidget::getInstance().getFileTransferW()->createFTWItem(fileName, determineFileType(fileInfo.suffix().toLower()), fileSize, "上传中");
}

// 下载文件
void WebDiskWiget::on_m_pDownloadFilePB_clicked()
{
    QTreeWidgetItem *item = ui->m_pFileInfoTreeW->currentItem();
    if (!item) {
        QMessageBox::warning(this, "下载文件", "请先选择文件");
        return;
    }

    QString savePath = QFileDialog::getSaveFileName(this, "保存文件", item->text(0));
    if (savePath.isEmpty()) return;

    FileRecv &fileRecv = TCPClient::getInstance().getFileRecv();
    if (fileRecv.file.isOpen()) {
        fileRecv.file.close();
    }
    fileRecv.file.setFileName(savePath);
    if (!fileRecv.file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "下载文件", "无法创建文件");
        return;
    }

    TCPClient::getInstance().setFileRecv();
    TCPClient::getInstance().startElapsedTimer();

    QString curPath = TCPClient::getInstance().getCurPath();
    std::shared_ptr<PDU> pdu = mk_shared_PDU(curPath.toUtf8().size() + 1);
    pdu->uiMsgType = ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST;
    strncpy(pdu->caData, item->text(0).toUtf8(), 64);
    memcpy(pdu->caMsg, curPath.toUtf8(), curPath.toUtf8().size());
    TCPClient::getInstance().getTCPSocket().write((char*)pdu.get(), pdu->uiPDULen);

    OpeWidget::getInstance().getFileTransferW()->createFTWItem(item->text(0), determineFileType(item->text(0).split('.').last().toLower()), 0, "下载中");
}

// 分享文件
void WebDiskWiget::on_m_pShareFilePB_clicked()
{
    QTreeWidgetItem *item = ui->m_pFileInfoTreeW->currentItem();
    if (!item) {
        QMessageBox::warning(this, "分享文件", "请先选择文件或文件夹");
        return;
    }

    m_shareFileName = item->text(0);
    ShareFileWidget::getInstance().show();
}

// 移动文件
void WebDiskWiget::on_m_pMoveFilePB_clicked()
{
    QTreeWidgetItem *item = ui->m_pFileInfoTreeW->currentItem();
    if (!item) {
        QMessageBox::warning(this, "移动文件", "请先选择文件或文件夹");
        return;
    }

    m_moveFilePath = TCPClient::getInstance().getCurPath() + "/" + item->text(0);
    FileSaveDialog::getInstance().setSaveFlag(false);
    FileSaveDialog::getInstance().setCurPath(TCPClient::getInstance().getRootPath());
    FileSaveDialog::getInstance().refreshDialog();
    FileSaveDialog::getInstance().show();
}

// 双击文件树
void WebDiskWiget::on_m_pFileInfoTreeW_doubleClicked(const QModelIndex &index)
{
    QString dirName = index.data().toString();
    QString curPath = TCPClient::getInstance().getCurPath();
    TCPClient::getInstance().setCurPathTemp(curPath + "/" + dirName);

    std::shared_ptr<PDU> pdu = mk_shared_PDU(curPath.toUtf8().size() + 1);
    pdu->uiMsgType = ENUM_MSG_TYPE_ENTER_DIR_REQUEST;
    strncpy(pdu->caData, dirName.toUtf8(), 64);
    memcpy(pdu->caMsg, curPath.toUtf8(), curPath.toUtf8().size());
    TCPClient::getInstance().getTCPSocket().write((char*)pdu.get(), pdu->uiPDULen);
}
