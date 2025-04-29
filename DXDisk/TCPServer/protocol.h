#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <memory>
#include <QtTypes>
#include <QFile>

typedef unsigned int uint;

// 错误和状态消息宏
#define UNKONW_ERROR "unknown error"
#define REGIST_OK "regist ok"
#define REGIST_FAILED "regist failed: name existed"
#define LOGIN_OK "login ok"
#define LOGIN_FAILED_NOEXIST "login failed: name not exist"
#define LOGIN_FAILED_PWDERROR "login failed: password error"
#define LOGIN_FAILED_RELOGIN "login failed: relogin"
#define SEARCH_USRNAME_NULL "search failed: name is null"
#define SEARCH_USR_NOEXIST "search failed: user not exist"
#define SEARCH_USR_OK "search ok"
#define ADD_FRIEND_EXITED "add friend failed: friend exist"
#define ADD_FRIEND_OFFLINE "add friend failed: user offline"
#define ADD_FRIEND_NOEXIST "add friend failed: user not exist"
#define ADD_FRIEND_SELF "add friend failed: add self as a friend"
#define ADD_FRIEND_SENT "add friend request has been sent"
#define DELETE_FRIEND_OK "delete friend ok"
#define DELETE_FRIEND_FAILED "delete friend failed"
#define SEND_MESSAGE_OK "send message ok"
#define SEND_MESSAGE_OFFLINE "send message failed: friend offline"
#define SEND_MESSAGE_NOFRIEND "send message failed: not friends"
#define SEND_MESSAGE_NOEXIST "send message failed: user not exist"
#define CREATE_DIR_OK "create dir ok"
#define CREATE_DIR_PATH_NOEXIST "create dir failed: path not exist"
#define CREATE_DIR_FILE_EXISTED "create dir failed: file existed"
#define DELETE_DIRORFILE_OK "delete ok"
#define DELETE_DIRORFILE_FAILED "delete failed"
#define RENAME_DIRORFILE_OK "rename ok"
#define RENAME_DIRORFILE_FAILED "rename failed"
#define ENTER_DIR_OK "enter dir ok"
#define ENTER_DIR_FAILED "enter dir failed: not a directory"
#define UPLOAD_FILE_OK "upload file ok"
#define UPLOAD_FILE_FAILED "upload file failed"
#define SAVE_FILE_OK "save file ok"
#define SAVE_FILE_FAILED "save file failed"
#define SAVE_FILE_FAILED_EXISTED "save file failed: file existed"
#define MOVE_FILE_OK "move file ok"
#define MOVE_FILE_FAILED "move file failed"

// 消息类型枚举
enum ENUM_MSG_TYPE
{
    ENUM_MSG_TYPE_MIN = 0,
    ENUM_MSG_TYPE_REGIST_REQUEST = 1,   // 注册请求
    ENUM_MSG_TYPE_REGIST_RESPOND,       // 注册回复
    ENUM_MSG_TYPE_LOGIN_REQUEST,        // 登录请求
    ENUM_MSG_TYPE_LOGIN_RESPOND,        // 登录回复
    ENUM_MSG_TYPE_ALL_ONLINEUSR_REQUEST,// 在线用户请求
    ENUM_MSG_TYPE_ALL_ONLINEUSR_RESPOND,// 在线用户回复
    ENUM_MSG_TYPE_SEARCH_USR_REQUEST,   // 搜索用户请求
    ENUM_MSG_TYPE_SEARCH_USR_RESPOND,   // 搜索用户回复
    ENUM_MSG_TYPE_ADD_FRIEND_REQUEST,   // 添加好友请求
    ENUM_MSG_TYPE_ADD_FRIEND_RESPOND,   // 添加好友回复
    ENUM_MSG_TYPE_ADD_FRIEND_AGREE_RESPOND, // 同意添加好友回复
    ENUM_MSG_TYPE_ADD_FRIEND_AGREE_RESPOND_RESPOND, // 同意添加好友回复的回复
    ENUM_MSG_TYPE_ADD_FRIEND_REFUSE_RESPOND, // 拒绝添加好友回复
    ENUM_MSG_TYPE_REFRESH_FRIEND_REQUEST,   // 刷新好友列表请求
    ENUM_MSG_TYPE_REFRESH_FRIEND_RESPOND,   // 刷新好友列表回复
    ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST,    // 删除好友请求
    ENUM_MSG_TYPE_DELETE_FRIEND_RESPOND,    // 删除好友回复
    ENUM_MSG_TYPE_SENDMSG_REQUEST,          // 发送消息请求
    ENUM_MSG_TYPE_SENDMSG_RESPOND,          // 发送消息回复
    ENUM_MSG_TYPE_MSGBOXCLICKED_RESPOND,    // 消息提示框确认回复
    ENUM_MSG_TYPE_SENDPUBLICMSG_REQUEST,    // 发送群聊消息请求
    ENUM_MSG_TYPE_CREATE_DIR_REQUEST,       // 创建文件夹请求
    ENUM_MSG_TYPE_CREATE_DIR_RESPOND,       // 创建文件夹回复
    ENUM_MSG_TYPE_REFRESH_DIR_REQUEST,      // 刷新文件夹请求
    ENUM_MSG_TYPE_REFRESH_DIR_RESPOND,      // 刷新文件夹回复
    ENUM_MSG_TYPE_DELETE_DIRORFILE_REQUEST, // 删除文件夹或文件请求
    ENUM_MSG_TYPE_DELETE_DIRORFILE_RESPOND, // 删除文件夹或文件回复
    ENUM_MSG_TYPE_RENAME_DIRORFILE_REQUEST, // 重命名文件夹或文件请求
    ENUM_MSG_TYPE_RENAME_DIRORFILE_RESPOND, // 重命名文件夹或文件回复
    ENUM_MSG_TYPE_ENTER_DIR_REQUEST,        // 进入文件夹请求
    ENUM_MSG_TYPE_ENTER_DIR_RESPOND,        // 进入文件夹回复
    ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST,      // 上传文件请求
    ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST_RESPOND, // 上传文件请求回复
    ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND,      // 上传文件回复
    ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST,    // 下载文件请求
    ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND,    // 下载文件回复
    ENUM_MSG_TYPE_SHARE_FILE_REQUEST,       // 分享文件请求
    ENUM_MSG_TYPE_REFRESH_SAVE_REQUEST,     // 保存窗口刷新请求
    ENUM_MSG_TYPE_REFRESH_SAVE_RESPOND,     // 保存窗口刷新回复
    ENUM_MSG_TYPE_ENTER_DIR_SAVE_REQUEST,   // 保存窗口进入文件夹请求
    ENUM_MSG_TYPE_ENTER_DIR_SAVE_RESPOND,   // 保存窗口进入文件夹回复
    ENUM_MSG_TYPE_CREATE_DIR_SAVE_REQUEST,  // 保存窗口创建文件夹请求
    ENUM_MSG_TYPE_CREATE_DIR_SAVE_RESPOND,  // 保存窗口创建文件夹回复
    ENUM_MSG_TYPE_SAVE_FILE_REQUEST,        // 保存文件请求
    ENUM_MSG_TYPE_SAVE_FILE_RESPOND,        // 保存文件回复
    ENUM_MSG_TYPE_MOVE_FILE_REQUEST,        // 移动文件请求
    ENUM_MSG_TYPE_MOVE_FILE_RESPOND,        // 移动文件回复
    ENUM_MSG_TYPE_MAX = 0x00ffffff
};

// 文件接收结构体
struct FileRecv
{
    QFile file;           // 文件对象
    qint64 totalSize = 0; // 文件总大小
    qint64 recvedSize = 0;// 已接收大小
    bool recvingFlag = false; // 接收状态
};

// 文件信息结构体
struct FileInfo
{
    char caFileNameUtf8[64]; // 文件名（UTF-8）
    int iFileType;           // 文件类型（0:文件夹，1+:文件）
    qint64 llFileSize;       // 文件大小
};

// 协议数据单元结构体
struct PDU
{
    uint uiPDULen;      // 总协议单元大小
    uint uiMsgType;     // 消息类型
    char caData[64];    // 数据字段
    uint uiMsgLen;      // 消息长度
    char caMsg[];       // 消息内容
};

// 创建共享PDU
std::shared_ptr<PDU> mk_shared_PDU(uint uiMsgLen);

// 调试显示PDU信息
void showPDUInfo(std::shared_ptr<PDU> pdu);

#endif // PROTOCOL_H
