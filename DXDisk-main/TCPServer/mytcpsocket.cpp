#include "mytcpsocket.h"
#include "mytcpserver.h"
#include "UniversalFunction.h"
#include "opedb.h"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>

// 构造函数，连接信号槽
MyTcpSocket::MyTcpSocket(QObject *parent)
    : QTcpSocket{parent}
{
    connect(this, &MyTcpSocket::readyRead, this, &MyTcpSocket::recvMsg);
    connect(this, &MyTcpSocket::disconnected, this, &MyTcpSocket::clientOffline);
}

// 生成刷新文件夹信息的PDU
std::shared_ptr<PDU> MyTcpSocket::refreshDirPDU(QString pCurPath)
{
    // 使用绝对路径，基于应用程序目录
    QString basePath = QCoreApplication::applicationDirPath() + "/UserWebDisk/";
    QString normalizedPath = QDir::cleanPath(basePath + pCurPath.section("user", -1));
    QDir dir(normalizedPath);

    // 检查目录权限
    QFileInfo dirInfo(normalizedPath);
    if (!dirInfo.isWritable()) {
        qDebug() << "refreshDirPDU: Directory not writable:" << normalizedPath;
        return mk_shared_PDU(0);
    }

    if (!dir.exists()) {
        qDebug() << "refreshDirPDU: Path does not exist:" << normalizedPath;
        if (dir.mkpath(normalizedPath)) {
            qDebug() << "refreshDirPDU: Created directory:" << normalizedPath;
        } else {
            qDebug() << "refreshDirPDU: Failed to create directory:" << normalizedPath;
            return mk_shared_PDU(0);
        }
    }

    QFileInfoList fileInfoList = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
    uint msgLen = sizeof(FileInfo) * fileInfoList.size();
    std::shared_ptr<PDU> pdu = mk_shared_PDU(msgLen);

    FileInfo *pFileInfo = nullptr;
    for (int i = 0; i < fileInfoList.size(); ++i) {
        pFileInfo = (FileInfo*)(pdu->caMsg) + i;
        QString fileName = fileInfoList[i].fileName();
        strncpy(pFileInfo->caFileNameUtf8, fileName.toUtf8(), qMin(fileName.toUtf8().size(), 63));

        if (fileInfoList[i].isDir()) {
            pFileInfo->iFileType = 0; // 文件夹
        } else {
            QString suffix = fileInfoList[i].suffix().toLower();
            pFileInfo->iFileType = determineFileType(suffix);
        }
        pFileInfo->llFileSize = fileInfoList[i].size();
    }

    return pdu;
}

// 处理客户端请求
void MyTcpSocket::handleRqsPDU(std::shared_ptr<PDU> pdu)
{
    if (!pdu) {
        qDebug() << "handleRqsPDU: Null PDU received";
        return;
    }

    switch (pdu->uiMsgType) {
    case ENUM_MSG_TYPE_REGIST_REQUEST: {
        char caName[32], caPwd[32];
        strncpy(caName, pdu->caData, 32);
        strncpy(caPwd, pdu->caData + 32, 32);
        bool ret = OpeDB::getInstance().handleRegist(caName, caPwd);
        std::shared_ptr<PDU> rpdPDU = mk_shared_PDU(0);
        rpdPDU->uiMsgType = ENUM_MSG_TYPE_REGIST_RESPOND;
        strcpy(rpdPDU->caData, ret ? REGIST_OK : REGIST_FAILED);
        if (ret) {
            QDir dir;
            QString userDir = QCoreApplication::applicationDirPath() + "/UserWebDisk/" + caName;
            if (!dir.exists(userDir)) {
                dir.mkpath(userDir);
                qDebug() << "Created user directory:" << userDir;
            }
        }
        write((char*)rpdPDU.get(), rpdPDU->uiPDULen);
        break;
    }
    case ENUM_MSG_TYPE_LOGIN_REQUEST: {
        char caName[32], caPwd[32];
        strncpy(caName, pdu->caData, 32);
        strncpy(caPwd, pdu->caData + 32, 32);
        uint ret = OpeDB::getInstance().handleLogin(caName, caPwd);
        std::shared_ptr<PDU> rpdPDU = mk_shared_PDU(0);
        rpdPDU->uiMsgType = ENUM_MSG_TYPE_LOGIN_RESPOND;
        if (ret == 0) {
            strcpy(rpdPDU->caData, LOGIN_OK);
            socketName = caName;
            QDir dir;
            QString userDir = QCoreApplication::applicationDirPath() + "/UserWebDisk/" + caName;
            if (!dir.exists(userDir)) {
                dir.mkpath(userDir);
                qDebug() << "Created user directory on login:" << userDir;
            }
        } else if (ret == 1) {
            strcpy(rpdPDU->caData, LOGIN_FAILED_NOEXIST);
        } else if (ret == 2) {
            strcpy(rpdPDU->caData, LOGIN_FAILED_PWDERROR);
        } else {
            strcpy(rpdPDU->caData, LOGIN_FAILED_RELOGIN);
        }
        write((char*)rpdPDU.get(), rpdPDU->uiPDULen);
        break;
    }
    case ENUM_MSG_TYPE_MSGBOXCLICKED_RESPOND: {
        write((char*)pdu.get(), pdu->uiPDULen);
        break;
    }
    case ENUM_MSG_TYPE_ALL_ONLINEUSR_REQUEST: {
        QStringList onlineUsr = OpeDB::getInstance().handeleAllOnlineUsr();
        uint uiMsgLen = onlineUsr.size() * 32;
        std::shared_ptr<PDU> rpdPDU = mk_shared_PDU(uiMsgLen);
        rpdPDU->uiMsgType = ENUM_MSG_TYPE_ALL_ONLINEUSR_RESPOND;
        for (int i = 0; i < onlineUsr.size(); ++i) {
            strncpy((char*)(rpdPDU->caMsg) + i * 32, onlineUsr[i].toUtf8(), 32);
        }
        write((char*)rpdPDU.get(), rpdPDU->uiPDULen);
        break;
    }
    case ENUM_MSG_TYPE_SEARCH_USR_REQUEST: {
        char searchUsrName[32];
        strncpy(searchUsrName, pdu->caData, 32);
        auto rearchResultMap = OpeDB::getInstance().handleSearchUsr(searchUsrName);
        uint uiMsgLen = rearchResultMap.size() * 34;
        std::shared_ptr<PDU> rpdPDU = mk_shared_PDU(uiMsgLen);
        rpdPDU->uiMsgType = ENUM_MSG_TYPE_SEARCH_USR_RESPOND;
        if (uiMsgLen == 0) {
            strcpy(rpdPDU->caData, SEARCH_USR_NOEXIST);
        } else {
            strcpy(rpdPDU->caData, SEARCH_USR_OK);
            uint i = 0;
            for (auto iter = rearchResultMap.constBegin(); iter != rearchResultMap.constEnd(); ++iter, ++i) {
                strncpy((char*)rpdPDU->caMsg + i * 34, iter.key().toUtf8(), 32);
                strncpy((char*)rpdPDU->caMsg + i * 34 + 32, iter.value().toUtf8(), 2);
            }
        }
        write((char*)rpdPDU.get(), rpdPDU->uiPDULen);
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST: {
        char perName[32], myName[32];
        strncpy(perName, pdu->caData, 32);
        strncpy(myName, pdu->caData + 32, 32);
        int ret = OpeDB::getInstance().queryUsrState(perName, myName);
        std::shared_ptr<PDU> rpdPDU = mk_shared_PDU(0);
        rpdPDU->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
        switch (ret) {
        case -5: strcpy(rpdPDU->caData, UNKONW_ERROR); break;
        case -4: strcpy(rpdPDU->caData, ADD_FRIEND_SELF); break;
        case -3: strcpy(rpdPDU->caData, ADD_FRIEND_NOEXIST); break;
        case -2: strcpy(rpdPDU->caData, ADD_FRIEND_OFFLINE); break;
        case -1: {
            strcpy(rpdPDU->caData, ADD_FRIEND_SENT);
            memcpy(rpdPDU->caData, perName, 32);
            memcpy(rpdPDU->caData + 32, myName, 32);
            MyTcpServer::getInstance().transpond(perName, pdu);
            break;
        }
        case 0:
        case 1: strcpy(rpdPDU->caData, ADD_FRIEND_EXITED); break;
        }
        write((char*)rpdPDU.get(), rpdPDU->uiPDULen);
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_AGREE_RESPOND: {
        char perName[32], myName[32];
        strncpy(perName, pdu->caData, 32);
        strncpy(myName, pdu->caData + 32, 32);
        bool ret = OpeDB::getInstance().handleAddFriend(perName, myName);
        if (ret) {
            auto myFriendMap = OpeDB::getInstance().handleRefFriend(myName);
            uint respondMsgLen = myFriendMap.size() * 34;
            std::shared_ptr<PDU> respondPDU = mk_shared_PDU(respondMsgLen);
            respondPDU->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_AGREE_RESPOND_RESPOND;
            uint i = 0;
            for (auto iter = myFriendMap.constBegin(); iter != myFriendMap.constEnd(); ++iter, ++i) {
                strncpy((char*)respondPDU->caMsg + i * 34, iter.key().toUtf8(), 32);
                strncpy((char*)respondPDU->caMsg + i * 34 + 32, iter.value().toUtf8(), 2);
            }
            write((char*)respondPDU.get(), respondPDU->uiPDULen);

            auto perFriendMap = OpeDB::getInstance().handleRefFriend(perName);
            uint transpondMsgLen = perFriendMap.size() * 34;
            std::shared_ptr<PDU> transpondPDU = mk_shared_PDU(transpondMsgLen);
            transpondPDU->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_AGREE_RESPOND;
            memcpy(transpondPDU->caData, pdu->caData, 64);
            i = 0;
            for (auto iter = perFriendMap.constBegin(); iter != perFriendMap.constEnd(); ++iter, ++i) {
                strncpy((char*)transpondPDU->caMsg + i * 34, iter.key().toUtf8(), 32);
                strncpy((char*)transpondPDU->caMsg + i * 34 + 32, iter.value().toUtf8(), 2);
            }
            MyTcpServer::getInstance().transpond(perName, transpondPDU);
        }
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_REFUSE_RESPOND: {
        MyTcpServer::getInstance().transpond(pdu->caData, pdu);
        break;
    }
    case ENUM_MSG_TYPE_REFRESH_FRIEND_REQUEST: {
        char name[32];
        strncpy(name, pdu->caData, 32);
        auto friendMap = OpeDB::getInstance().handleRefFriend(name);
        uint uiMsgLen = friendMap.size() * 34;
        std::shared_ptr<PDU> rpdPDU = mk_shared_PDU(uiMsgLen);
        rpdPDU->uiMsgType = ENUM_MSG_TYPE_REFRESH_FRIEND_RESPOND;
        uint i = 0;
        for (auto iter = friendMap.constBegin(); iter != friendMap.constEnd(); ++iter, ++i) {
            strncpy((char*)rpdPDU->caMsg + i * 34, iter.key().toUtf8(), 32);
            strncpy((char*)rpdPDU->caMsg + i * 34 + 32, iter.value().toUtf8(), 2);
        }
        write((char*)rpdPDU.get(), rpdPDU->uiPDULen);
        break;
    }
    case ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST: {
        bool ret = OpeDB::getInstance().handleDelFriend(pdu->caData, pdu->caData + 32);
        char myName[32];
        memcpy(myName, pdu->caData + 32, 32);
        auto myFriendMap = OpeDB::getInstance().handleRefFriend(myName);
        uint respondMsgLen = myFriendMap.size() * 34;
        std::shared_ptr<PDU> respondPDU = mk_shared_PDU(respondMsgLen);
        respondPDU->uiMsgType = ENUM_MSG_TYPE_DELETE_FRIEND_RESPOND;
        uint i = 0;
        for (auto iter = myFriendMap.constBegin(); iter != myFriendMap.constEnd(); ++iter, ++i) {
            strncpy((char*)respondPDU->caMsg + i * 34, iter.key().toUtf8(), 32);
            strncpy((char*)respondPDU->caMsg + i * 34 + 32, iter.value().toUtf8(), 2);
        }
        strcpy(respondPDU->caData, ret ? DELETE_FRIEND_OK : DELETE_FRIEND_FAILED);
        memcpy(respondPDU->caData + 32, pdu->caData, 32);
        write((char*)respondPDU.get(), respondPDU->uiPDULen);
        break;
    }
    case ENUM_MSG_TYPE_SENDMSG_REQUEST: {
        std::shared_ptr<PDU> rpdPDU = mk_shared_PDU(0);
        rpdPDU->uiMsgType = ENUM_MSG_TYPE_SENDMSG_RESPOND;
        int ret = OpeDB::getInstance().queryUsrState(pdu->caData, pdu->caData + 32);
        switch (ret) {
        case 1: {
            strcpy(rpdPDU->caData, SEND_MESSAGE_OK);
            MyTcpServer::getInstance().transpond(pdu->caData, pdu);
            break;
        }
        case 0: strcpy(rpdPDU->caData, SEND_MESSAGE_OFFLINE); break;
        case -1:
        case -2: strcpy(rpdPDU->caData, SEND_MESSAGE_NOFRIEND); break;
        case -3: strcpy(rpdPDU->caData, SEND_MESSAGE_NOEXIST); break;
        default: strcpy(rpdPDU->caData, UNKONW_ERROR); break;
        }
        write((char*)rpdPDU.get(), rpdPDU->uiPDULen);
        break;
    }
    case ENUM_MSG_TYPE_SENDPUBLICMSG_REQUEST: {
        auto onlineUsrList = OpeDB::getInstance().handeleAllOnlineUsr();
        for (const QString &usr : onlineUsrList) {
            MyTcpServer::getInstance().transpond(usr, pdu);
        }
        break;
    }
    case ENUM_MSG_TYPE_CREATE_DIR_REQUEST:
    case ENUM_MSG_TYPE_CREATE_DIR_SAVE_REQUEST: {
        QDir dir;
        QString strCurPath = QString::fromUtf8(pdu->caMsg, pdu->uiMsgLen);
        QString normalizedPath = QCoreApplication::applicationDirPath() + "/UserWebDisk/" + strCurPath.section("user", -1);
        normalizedPath = QDir::cleanPath(normalizedPath);
        bool ret = dir.exists(normalizedPath);
        std::shared_ptr<PDU> rpdPDU = mk_shared_PDU(0);
        rpdPDU->uiMsgType = (pdu->uiMsgType == ENUM_MSG_TYPE_CREATE_DIR_REQUEST) ?
                                ENUM_MSG_TYPE_CREATE_DIR_RESPOND : ENUM_MSG_TYPE_CREATE_DIR_SAVE_RESPOND;
        if (!ret) {
            ret = dir.mkpath(normalizedPath);
            if (!ret) {
                qDebug() << "createDir: Failed to create path:" << normalizedPath;
                strcpy(rpdPDU->caData, CREATE_DIR_PATH_NOEXIST);
                write((char*)rpdPDU.get(), rpdPDU->uiPDULen);
                break;
            }
            qDebug() << "createDir: Created path:" << normalizedPath;
        }
        char caNewDir[64];
        strncpy(caNewDir, pdu->caData, 64);
        QString strNewPath = normalizedPath + "/" + caNewDir;
        ret = dir.exists(strNewPath);
        if (ret) {
            strcpy(rpdPDU->caData, CREATE_DIR_FILE_EXISTED);
            write((char*)rpdPDU.get(), rpdPDU->uiPDULen);
        } else {
            dir.mkdir(strNewPath);
            std::shared_ptr<PDU> rpdPDU2 = refreshDirPDU(strCurPath);
            if (rpdPDU2) {
                rpdPDU2->uiMsgType = rpdPDU->uiMsgType;
                strcpy(rpdPDU2->caData, CREATE_DIR_OK);
                write((char*)rpdPDU2.get(), rpdPDU2->uiPDULen);
            } else {
                qDebug() << "createDir: Failed to refresh directory:" << normalizedPath;
                strcpy(rpdPDU->caData, CREATE_DIR_PATH_NOEXIST);
                write((char*)rpdPDU.get(), rpdPDU->uiPDULen);
            }
        }
        break;
    }
    case ENUM_MSG_TYPE_REFRESH_DIR_REQUEST:
    case ENUM_MSG_TYPE_REFRESH_SAVE_REQUEST: {
        QString pCurPath = QString::fromUtf8(pdu->caMsg, pdu->uiMsgLen);
        std::shared_ptr<PDU> rpdPDU = refreshDirPDU(pCurPath);
        if (rpdPDU) {
            rpdPDU->uiMsgType = (pdu->uiMsgType == ENUM_MSG_TYPE_REFRESH_DIR_REQUEST) ?
                                    ENUM_MSG_TYPE_REFRESH_DIR_RESPOND : ENUM_MSG_TYPE_REFRESH_SAVE_RESPOND;
            write((char*)rpdPDU.get(), rpdPDU->uiPDULen);
        } else {
            qDebug() << "refreshDir: Failed to generate PDU for path:" << pCurPath;
            std::shared_ptr<PDU> errorPDU = mk_shared_PDU(0);
            errorPDU->uiMsgType = (pdu->uiMsgType == ENUM_MSG_TYPE_REFRESH_DIR_REQUEST) ?
                                      ENUM_MSG_TYPE_REFRESH_DIR_RESPOND : ENUM_MSG_TYPE_REFRESH_SAVE_RESPOND;
            strcpy(errorPDU->caData, "refresh failed");
            write((char*)errorPDU.get(), errorPDU->uiPDULen);
        }
        break;
    }
    case ENUM_MSG_TYPE_DELETE_DIRORFILE_REQUEST: {
        char caName[64];
        strncpy(caName, pdu->caData, 64);
        QString delPath = QString("%1/%2").arg(QString::fromUtf8(pdu->caMsg, pdu->uiMsgLen)).arg(caName);
        QString normalizedPath = QCoreApplication::applicationDirPath() + "/UserWebDisk/" + delPath.section("user", -1);
        normalizedPath = QDir::cleanPath(normalizedPath);
        QFileInfo fileInfo(normalizedPath);
        bool ret = false;
        QDir dir;
        if (fileInfo.isDir()) {
            dir.setPath(normalizedPath);
            ret = dir.removeRecursively();
        } else if (fileInfo.isFile()) {
            ret = dir.remove(normalizedPath);
        }
        std::shared_ptr<PDU> rpdPDU = refreshDirPDU(QString::fromUtf8(pdu->caMsg, pdu->uiMsgLen));
        if (rpdPDU) {
            rpdPDU->uiMsgType = ENUM_MSG_TYPE_DELETE_DIRORFILE_RESPOND;
            strcpy(rpdPDU->caData, ret ? DELETE_DIRORFILE_OK : DELETE_DIRORFILE_FAILED);
            write((char*)rpdPDU.get(), rpdPDU->uiPDULen);
        } else {
            qDebug() << "deleteDirOrFile: Failed to refresh directory:" << delPath;
            std::shared_ptr<PDU> errorPDU = mk_shared_PDU(0);
            errorPDU->uiMsgType = ENUM_MSG_TYPE_DELETE_DIRORFILE_RESPOND;
            strcpy(errorPDU->caData, DELETE_DIRORFILE_FAILED);
            write((char*)errorPDU.get(), errorPDU->uiPDULen);
        }
        break;
    }
    case ENUM_MSG_TYPE_RENAME_DIRORFILE_REQUEST: {
        QString newName = QString::fromUtf8(pdu->caData);
        QString oldName = QString::fromUtf8(pdu->caMsg);
        QString curPath = QString::fromUtf8(pdu->caMsg + oldName.toUtf8().size() + 1);
        QString normalizedPath = QCoreApplication::applicationDirPath() + "/UserWebDisk/" + curPath.section("user", -1);
        normalizedPath = QDir::cleanPath(normalizedPath);
        QString oldPath = QString("%1/%2").arg(normalizedPath).arg(oldName);
        QString newPath = QString("%1/%2").arg(normalizedPath).arg(newName);
        QDir dir;
        bool ret = dir.rename(oldPath, newPath);
        std::shared_ptr<PDU> rpdPDU = refreshDirPDU(curPath);
        if (rpdPDU) {
            rpdPDU->uiMsgType = ENUM_MSG_TYPE_RENAME_DIRORFILE_RESPOND;
            strcpy(rpdPDU->caData, ret ? RENAME_DIRORFILE_OK : RENAME_DIRORFILE_FAILED);
            write((char*)rpdPDU.get(), rpdPDU->uiPDULen);
        } else {
            qDebug() << "renameDirOrFile: Failed to refresh directory:" << curPath;
            std::shared_ptr<PDU> errorPDU = mk_shared_PDU(0);
            errorPDU->uiMsgType = ENUM_MSG_TYPE_RENAME_DIRORFILE_RESPOND;
            strcpy(errorPDU->caData, RENAME_DIRORFILE_FAILED);
            write((char*)errorPDU.get(), errorPDU->uiPDULen);
        }
        break;
    }
    case ENUM_MSG_TYPE_ENTER_DIR_REQUEST:
    case ENUM_MSG_TYPE_ENTER_DIR_SAVE_REQUEST: {
        QString dirName = QString::fromUtf8(pdu->caData);
        QString curPath = QString::fromUtf8(pdu->caMsg, pdu->uiMsgLen);
        QString enterDirPath = curPath + "/" + dirName;
        QString normalizedPath = QCoreApplication::applicationDirPath() + "/UserWebDisk/" + enterDirPath.section("user", -1);
        normalizedPath = QDir::cleanPath(normalizedPath);
        QFileInfo fileInfo(normalizedPath);
        std::shared_ptr<PDU> rpdPDU;
        if (fileInfo.isDir()) {
            rpdPDU = refreshDirPDU(enterDirPath);
            if (rpdPDU) {
                rpdPDU->uiMsgType = (pdu->uiMsgType == ENUM_MSG_TYPE_ENTER_DIR_REQUEST) ?
                                        ENUM_MSG_TYPE_ENTER_DIR_RESPOND : ENUM_MSG_TYPE_ENTER_DIR_SAVE_RESPOND;
                strcpy(rpdPDU->caData, ENTER_DIR_OK);
                write((char*)rpdPDU.get(), rpdPDU->uiPDULen);
            } else {
                qDebug() << "enterDir: Failed to refresh directory:" << normalizedPath;
                rpdPDU = mk_shared_PDU(0);
                rpdPDU->uiMsgType = (pdu->uiMsgType == ENUM_MSG_TYPE_ENTER_DIR_REQUEST) ?
                                        ENUM_MSG_TYPE_ENTER_DIR_RESPOND : ENUM_MSG_TYPE_ENTER_DIR_SAVE_RESPOND;
                strcpy(rpdPDU->caData, ENTER_DIR_FAILED);
                write((char*)rpdPDU.get(), rpdPDU->uiPDULen);
            }
        } else {
            rpdPDU = mk_shared_PDU(0);
            rpdPDU->uiMsgType = (pdu->uiMsgType == ENUM_MSG_TYPE_ENTER_DIR_REQUEST) ?
                                    ENUM_MSG_TYPE_ENTER_DIR_RESPOND : ENUM_MSG_TYPE_ENTER_DIR_SAVE_RESPOND;
            strcpy(rpdPDU->caData, ENTER_DIR_FAILED);
            write((char*)rpdPDU.get(), rpdPDU->uiPDULen);
        }
        break;
    }
    case ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST: {
        char uploadFileName[64];
        strncpy(uploadFileName, pdu->caData, 64);
        QString msgContent = QString::fromUtf8(pdu->caMsg, pdu->uiMsgLen);
        int pos = msgContent.indexOf(' ');
        if (pos == -1) {
            qDebug() << "Invalid upload file size format";
            std::shared_ptr<PDU> rpdPDU = mk_shared_PDU(0);
            rpdPDU->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND;
            strcpy(rpdPDU->caData, UPLOAD_FILE_FAILED);
            write((char*)rpdPDU.get(), rpdPDU->uiPDULen);
            break;
        }
        qint64 uploadFileSize = msgContent.left(pos).toLongLong();
        QString uploadPath = msgContent.mid(pos + 1);
        QString normalizedPath = QCoreApplication::applicationDirPath() + "/UserWebDisk/" + uploadPath.section("user", -1);
        normalizedPath = QDir::cleanPath(normalizedPath);
        QString uploadFilePath = QString("%1/%2").arg(normalizedPath).arg(uploadFileName);

        // 确保上传路径存在
        QDir dir;
        if (!dir.exists(normalizedPath)) {
            if (!dir.mkpath(normalizedPath)) {
                qDebug() << "Failed to create upload directory:" << normalizedPath;
                std::shared_ptr<PDU> rpdPDU = mk_shared_PDU(0);
                rpdPDU->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND;
                strcpy(rpdPDU->caData, UPLOAD_FILE_FAILED);
                write((char*)rpdPDU.get(), rpdPDU->uiPDULen);
                break;
            }
            qDebug() << "Created upload directory:" << normalizedPath;
        }

        if (fileRecv.file.isOpen()) {
            fileRecv.file.close();
        }
        fileRecv.file.setFileName(uploadFilePath);
        if (!fileRecv.file.open(QIODevice::WriteOnly)) {
            qDebug() << "Failed to open file for writing:" << uploadFilePath;
            std::shared_ptr<PDU> rpdPDU = mk_shared_PDU(0);
            rpdPDU->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND;
            strcpy(rpdPDU->caData, UPLOAD_FILE_FAILED);
            write((char*)rpdPDU.get(), rpdPDU->uiPDULen);
            break;
        }

        fileRecv.recvingFlag = true;
        fileRecv.totalSize = uploadFileSize;
        fileRecv.recvedSize = 0;
        m_curPath = uploadPath;
        qDebug() << "Server ready to receive file:" << uploadFilePath;

        std::shared_ptr<PDU> rpdPDU = mk_shared_PDU(0);
        rpdPDU->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST_RESPOND;
        write((char*)rpdPDU.get(), rpdPDU->uiPDULen);
        break;
    }
    case ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST: {
        QString downloadFilePath = QString("%1/%2").arg(QString::fromUtf8(pdu->caMsg, pdu->uiMsgLen)).arg(pdu->caData);
        QString normalizedPath = QCoreApplication::applicationDirPath() + "/UserWebDisk/" + downloadFilePath.section("user", -1);
        normalizedPath = QDir::cleanPath(normalizedPath);
        QFile file(normalizedPath);
        if (file.open(QIODevice::ReadOnly)) {
            char *pBuffer = new char[4096];
            qint64 ret;
            while ((ret = file.read(pBuffer, 4096)) > 0) {
                write(pBuffer, ret);
                waitForBytesWritten(1000);
            }
            if (ret < 0) {
                qDebug() << "Failed to read file:" << normalizedPath;
            }
            file.close();
            delete[] pBuffer;
        } else {
            qDebug() << "Failed to open file for reading:" << normalizedPath;
        }
        break;
    }
    case ENUM_MSG_TYPE_SHARE_FILE_REQUEST: {
        QString shareFilePath = QString::fromUtf8(pdu->caMsg);
        QString normalizedPath = QCoreApplication::applicationDirPath() + "/UserWebDisk/" + shareFilePath.section("user", -1);
        normalizedPath = QDir::cleanPath(normalizedPath);
        int index = normalizedPath.lastIndexOf('/');
        QString shareFileName = normalizedPath.mid(index + 1);
        int recverNumPos = shareFilePath.toUtf8().size() + 1;
        QString recverNumStr = QString::fromUtf8(pdu->caMsg + recverNumPos);
        int recverNum = recverNumStr.toInt();

        QFileInfo fileInfo(normalizedPath);
        std::shared_ptr<PDU> rpdPDU = mk_shared_PDU(sizeof(FileInfo) + shareFilePath.toUtf8().size() + 1);
        rpdPDU->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_REQUEST;
        strcpy(rpdPDU->caData, pdu->caData);

        FileInfo *pFileInfo = (FileInfo*)(rpdPDU->caMsg);
        strncpy(pFileInfo->caFileNameUtf8, shareFileName.toUtf8(), 63);
        pFileInfo->iFileType = fileInfo.isDir() ? 0 : determineFileType(fileInfo.suffix().toLower());
        pFileInfo->llFileSize = fileInfo.size();
        memcpy(rpdPDU->caMsg + sizeof(FileInfo), pdu->caMsg, shareFilePath.toUtf8().size());

        for (int i = 0; i < recverNum; ++i) {
            char recverName[64];
            strncpy(recverName, pdu->caMsg + recverNumPos + sizeof(recverNumStr.toInt()) + 1 + i * 64, 64);
            MyTcpServer::getInstance().transpond(recverName, rpdPDU);
        }
        break;
    }
    case ENUM_MSG_TYPE_SAVE_FILE_REQUEST: {
        QString savePath = QString::fromUtf8(pdu->caMsg);
        QString normalizedSavePath = QCoreApplication::applicationDirPath() + "/UserWebDisk/" + savePath.section("user", -1);
        normalizedSavePath = QDir::cleanPath(normalizedSavePath);
        QString shareFilePath = QString::fromUtf8(pdu->caMsg + savePath.toUtf8().size() + 1);
        QString normalizedSharePath = QCoreApplication::applicationDirPath() + "/UserWebDisk/" + shareFilePath.section("user", -1);
        normalizedSharePath = QDir::cleanPath(normalizedSharePath);
        int index = normalizedSharePath.lastIndexOf('/');
        QString shareFileName = normalizedSharePath.mid(index + 1);
        QString saveFilePath = normalizedSavePath + "/" + shareFileName;

        std::shared_ptr<PDU> rpdPDU = mk_shared_PDU(0);
        rpdPDU->uiMsgType = ENUM_MSG_TYPE_SAVE_FILE_RESPOND;

        QDir dir;
        bool ret = dir.exists(saveFilePath);
        if (ret) {
            strcpy(rpdPDU->caData, SAVE_FILE_FAILED_EXISTED);
        } else {
            QFileInfo fileInfo(normalizedSharePath);
            if (fileInfo.isFile()) {
                ret = QFile::copy(normalizedSharePath, saveFilePath);
            } else if (fileInfo.isDir()) {
                ret = copyDir(normalizedSharePath, saveFilePath);
            }
            strcpy(rpdPDU->caData, ret ? SAVE_FILE_OK : SAVE_FILE_FAILED);
        }
        write((char*)rpdPDU.get(), rpdPDU->uiPDULen);
        break;
    }
    case ENUM_MSG_TYPE_MOVE_FILE_REQUEST: {
        QString destPath = QString::fromUtf8(pdu->caMsg);
        QString normalizedDestPath = QCoreApplication::applicationDirPath() + "/UserWebDisk/" + destPath.section("user", -1);
        normalizedDestPath = QDir::cleanPath(normalizedDestPath);
        QString srcPath = QString::fromUtf8(pdu->caMsg + destPath.toUtf8().size() + 1);
        QString normalizedSrcPath = QCoreApplication::applicationDirPath() + "/UserWebDisk/" + srcPath.section("user", -1);
        normalizedSrcPath = QDir::cleanPath(normalizedSrcPath);
        int index = normalizedSrcPath.lastIndexOf('/');
        QString moveFileName = normalizedSrcPath.mid(index + 1);
        QString destFilePath = normalizedDestPath + "/" + moveFileName;

        bool ret = QFile::rename(normalizedSrcPath, destFilePath);
        std::shared_ptr<PDU> rpdPDU = mk_shared_PDU(0);
        rpdPDU->uiMsgType = ENUM_MSG_TYPE_MOVE_FILE_RESPOND;
        strcpy(rpdPDU->caData, ret ? MOVE_FILE_OK : MOVE_FILE_FAILED);
        write((char*)rpdPDU.get(), rpdPDU->uiPDULen);
        break;
    }
    default:
        qDebug() << "Unknown MsgType:" << pdu->uiMsgType;
        break;
    }
}

// 获取用户名
QString MyTcpSocket::getName()
{
    return socketName;
}

// 拷贝文件夹
bool MyTcpSocket::copyDir(QString srcDir, QString destDir)
{
    QDir dir;
    bool ret1 = dir.mkdir(destDir);
    bool ret2 = true, ret3 = true;
    dir.setPath(srcDir);
    QFileInfoList fileInfoList = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
    for (const QFileInfo &info : fileInfoList) {
        if (info.isFile()) {
            ret2 = QFile::copy(srcDir + "/" + info.fileName(), destDir + "/" + info.fileName());
        } else if (info.isDir()) {
            ret3 = copyDir(srcDir + "/" + info.fileName(), destDir + "/" + info.fileName());
        }
    }
    return ret1 && ret2 && ret3;
}

// 接收消息
void MyTcpSocket::recvMsg()
{
    if (!fileRecv.recvingFlag) {
        while (bytesAvailable() >= static_cast<qint64>(sizeof(uint))) {
            uint uiPDULen = 0;
            peek((char*)&uiPDULen, sizeof(uint));
            if (bytesAvailable() < static_cast<qint64>(uiPDULen)) {
                qDebug() << "recvMsg: Incomplete PDU, waiting for more data";
                return;
            }
            read((char*)&uiPDULen, sizeof(uint));
            uint uiMsgLen = uiPDULen - sizeof(PDU);
            std::shared_ptr<PDU> pdu = mk_shared_PDU(uiMsgLen);
            pdu->uiPDULen = uiPDULen;
            read((char*)pdu.get() + sizeof(uint), uiPDULen - sizeof(uint));
            showPDUInfo(pdu);
            handleRqsPDU(pdu);
        }
    } else {
        QByteArray recvBuff = readAll();
        fileRecv.file.write(recvBuff);
        fileRecv.recvedSize += recvBuff.size();
        qDebug() << "Uploaded:" << QString("%1%").arg((double)fileRecv.recvedSize / fileRecv.totalSize * 100, 0, 'f', 2);
        if (fileRecv.recvedSize >= fileRecv.totalSize) {
            fileRecv.file.close();
            fileRecv.recvingFlag = false;
            std::shared_ptr<PDU> rpdPDU = mk_shared_PDU(0);
            rpdPDU->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND;
            strcpy(rpdPDU->caData, fileRecv.recvedSize == fileRecv.totalSize ? UPLOAD_FILE_OK : UPLOAD_FILE_FAILED);
            fileRecv.recvedSize = 0;
            fileRecv.totalSize = 0;
            write((char*)rpdPDU.get(), rpdPDU->uiPDULen);
            // 刷新上传目录
            std::shared_ptr<PDU> refreshPDU = refreshDirPDU(m_curPath);
            if (refreshPDU) {
                refreshPDU->uiMsgType = ENUM_MSG_TYPE_REFRESH_DIR_RESPOND;
                write((char*)refreshPDU.get(), refreshPDU->uiPDULen);
            }
        }
    }
}

// 处理客户端下线
void MyTcpSocket::clientOffline()
{
    OpeDB::getInstance().handeleOffline(socketName.toUtf8().constData());
    emit offline(this);
}
