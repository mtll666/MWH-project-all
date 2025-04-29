#ifndef WEBDISKWIGET_H
#define WEBDISKWIGET_H

#include <QWidget>
#include <QTreeWidget>
#include "protocol.h"
#include <QFile>

namespace Ui {
class WebDiskWiget;
}

class WebDiskWiget : public QWidget
{
    Q_OBJECT

public:
    explicit WebDiskWiget(QWidget *parent = nullptr);
    ~WebDiskWiget();
    void updateFileInfo(std::shared_ptr<PDU> pdu); // 更新文件信息
    void uploadFileData(); // 发送上传文件数据
    void handleUploadFileResponse(std::shared_ptr<PDU> pdu); // 处理上传文件回复
    void handleDownloadFileResponse(std::shared_ptr<PDU> pdu); // 处理下载文件回复
    void handleSaveFileResponse(std::shared_ptr<PDU> pdu); // 处理保存文件回复
    void handleMoveFileResponse(std::shared_ptr<PDU> pdu); // 处理移动文件回复
    QString getShareFileName(); // 获取分享文件名
    QString getMoveFilePath(); // 获取移动文件路径
    void refreshDir(); // 刷新文件夹

private slots:
    void on_m_pReturnPB_clicked(); // 返回按钮
    void on_m_pCreateDirPB_clicked(); // 新建文件夹
    void on_m_pDeletePB_clicked(); // 删除文件
    void on_m_pRefreshDirPB_clicked(); // 刷新文件夹
    void on_m_pRenameFilePB_clicked(); // 重命名文件
    void on_m_pUploadFilePB_clicked(); // 上传文件
    void on_m_pDownloadFilePB_clicked(); // 下载文件
    void on_m_pShareFilePB_clicked(); // 分享文件
    void on_m_pMoveFilePB_clicked(); // 移动文件
    void on_m_pFileInfoTreeW_doubleClicked(const QModelIndex &index); // 双击文件树

private:
    Ui::WebDiskWiget *ui;
    QString m_shareFileName; // 分享文件名
    QString m_moveFilePath; // 移动文件路径
    QFile m_uploadFile; // 上传文件对象
};

#endif // WEBDISKWIGET_H
