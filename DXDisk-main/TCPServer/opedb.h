#ifndef OPEDB_H
#define OPEDB_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError> // 添加 QSqlError 头文件以解决 incomplete type 错误

class OpeDB : public QObject
{
    Q_OBJECT
public:
    explicit OpeDB(QObject *parent = nullptr);
    static OpeDB& getInstance();
    void init();
    ~OpeDB();
    bool handleRegist(const char *name, const char *pwd);    // 处理注册函数
    // 处理登录函数：返回0表示登录成功，返回1表示未注册，返回2表示密码错误，返回3表示用户已在线
    uint handleLogin(const char *name, const char *pwd);
    void handeleOffline(const char *name);      // 处理客户端下线函数
    QStringList handeleAllOnlineUsr();     // 处理查询所有在线用户的函数
    // 处理搜索用户函数，根据搜索用户名前缀查找
    QMap<QString,QString> handleSearchUsr(const char *searchUsrName_pre);
    // 查询用户状态，返回值-5故障,-4对方是自己,-3对方已注销不存在
    // 表示双方不是好友:-2对方不在线,-1对方在线
    // 表示双方是好友:0对方不在线，1对方在线
    int queryUsrState(const char *perUsrName, const char *myName);
    bool handleAddFriend(const char *perUsrName, const char *myName);  // 处理添加好友函数
    // 处理刷新好友列表函数，返回好友名和在线状态
    QMap<QString,QString> handleRefFriend(const char *name);
    bool handleDelFriend(const char *perUsrName, const char *myName);  // 处理删除好友函数

signals:

private:
    QSqlDatabase m_db;  // 用来连接数据库
};

#endif // OPEDB_H
