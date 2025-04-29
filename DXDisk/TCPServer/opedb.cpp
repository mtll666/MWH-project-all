#include "opedb.h"
#include <QMessageBox>
#include <QDebug>

// 构造函数，初始化 MySQL 数据库
OpeDB::OpeDB(QObject *parent)
    : QObject{parent}
{
    m_db = QSqlDatabase::addDatabase("QMYSQL");
}

// 单例模式
OpeDB &OpeDB::getInstance()
{
    static OpeDB instance;
    return instance;
}

// 初始化数据库连接
void OpeDB::init()
{
    m_db.setHostName("localhost");
    m_db.setDatabaseName("dxdisk");
    m_db.setUserName("root");
    m_db.setPassword("123456");
    if (m_db.open()) {
        qDebug() << "(OpeDB::init) 数据库打开成功";
    } else {
        QMessageBox::critical(nullptr, "打开数据库", "打开数据库失败: " + m_db.lastError().text());
        qDebug() << "(OpeDB::init) 数据库打开失败:" << m_db.lastError().text();
    }
}

// 析构函数，关闭数据库连接
OpeDB::~OpeDB()
{
    m_db.close();
}

// 处理注册
bool OpeDB::handleRegist(const char *name, const char *pwd)
{
    if (!name || !pwd) return false;
    QString command = QString("INSERT INTO usrInfo (name, pwd, online) VALUES ('%1', '%2', 0)")
                          .arg(name).arg(pwd);
    qDebug() << "(OpeDB::handleRegist) 执行SQL:" << command;
    QSqlQuery query;
    bool ret = query.exec(command);
    if (!ret) {
        qDebug() << "(OpeDB::handleRegist) 注册失败:" << query.lastError().text();
    }
    return ret;
}

// 处理登录：0成功，1未注册，2密码错误，3已在线
uint OpeDB::handleLogin(const char *name, const char *pwd)
{
    if (!name || !pwd) return 1;
    QString command = QString("SELECT * FROM usrInfo WHERE name='%1'").arg(name);
    qDebug() << "(OpeDB::handleLogin) 执行SQL:" << command;
    QSqlQuery query;
    if (!query.exec(command)) {
        qDebug() << "(OpeDB::handleLogin) 查询失败:" << query.lastError().text();
        return 1;
    }
    if (!query.next()) return 1; // 用户不存在
    if (query.value("pwd").toString() != pwd) return 2; // 密码错误
    if (query.value("online").toInt() == 1) return 3; // 已在线
    // 更新在线状态
    command = QString("UPDATE usrInfo SET online=1 WHERE name='%1' AND pwd='%2'")
                  .arg(name).arg(pwd);
    qDebug() << "(OpeDB::handleLogin) 执行SQL:" << command;
    bool ret = query.exec(command);
    if (!ret) {
        qDebug() << "(OpeDB::handleLogin) 更新在线状态失败:" << query.lastError().text();
    }
    return 0;
}

// 处理客户端下线
void OpeDB::handeleOffline(const char *name)
{
    if (!name) return;
    QString command = QString("UPDATE usrInfo SET online=0 WHERE name='%1'").arg(name);
    qDebug() << "(OpeDB::handeleOffline) 执行SQL:" << command;
    QSqlQuery query;
    if (!query.exec(command)) {
        qDebug() << "(OpeDB::handeleOffline) 更新离线状态失败:" << query.lastError().text();
    }
}

// 查询所有在线用户
QStringList OpeDB::handeleAllOnlineUsr()
{
    QString command = QString("SELECT name FROM usrInfo WHERE online=1");
    qDebug() << "(OpeDB::handeleAllOnlineUsr) 执行SQL:" << command;
    QSqlQuery query;
    QStringList onlineUsr;
    if (!query.exec(command)) {
        qDebug() << "(OpeDB::handeleAllOnlineUsr) 查询失败:" << query.lastError().text();
        return onlineUsr;
    }
    while (query.next()) {
        onlineUsr.append(query.value(0).toString());
    }
    return onlineUsr;
}

// 搜索用户
QMap<QString, QString> OpeDB::handleSearchUsr(const char *searchUsrName_pre)
{
    QMap<QString, QString> searchResultMap;
    if (!searchUsrName_pre) return searchResultMap;
    QString command = QString("SELECT name, online FROM usrInfo WHERE name LIKE '%%%1%%'")
                          .arg(searchUsrName_pre);
    qDebug() << "(OpeDB::handleSearchUsr) 执行SQL:" << command;
    QSqlQuery query;
    if (!query.exec(command)) {
        qDebug() << "(OpeDB::handleSearchUsr) 查询失败:" << query.lastError().text();
        return searchResultMap;
    }
    while (query.next()) {
        searchResultMap.insert(query.value("name").toString(), query.value("online").toString());
    }
    return searchResultMap;
}

// 查询用户状态
int OpeDB::queryUsrState(const char *perUsrName, const char *myName)
{
    if (!perUsrName || !myName) return -5;
    if (strcmp(perUsrName, myName) == 0) return -4;
    QString queryFriCommand = QString("SELECT * FROM friendInfo WHERE "
                                      "(name='%1' AND fname='%2') OR (name='%2' AND fname='%1')")
                                  .arg(perUsrName).arg(myName);
    QString queryStaCommand = QString("SELECT online FROM usrInfo WHERE name='%1'")
                                  .arg(perUsrName);
    QSqlQuery queryFriResult;
    QSqlQuery queryStaResult;
    if (!queryFriResult.exec(queryFriCommand)) {
        qDebug() << "(OpeDB::queryUsrState) 查询好友关系失败:" << queryFriResult.lastError().text();
        return -5;
    }
    if (!queryStaResult.exec(queryStaCommand)) {
        qDebug() << "(OpeDB::queryUsrState) 查询用户状态失败:" << queryStaResult.lastError().text();
        return -5;
    }
    if (!queryFriResult.next()) {
        if (queryStaResult.next()) {
            return queryStaResult.value(0).toInt() - 2; // -2:不在线, -1:在线
        } else {
            return -3; // 用户不存在
        }
    } else {
        if (queryStaResult.next()) {
            return queryStaResult.value(0).toInt(); // 0:不在线, 1:在线
        } else {
            return -5;
        }
    }
}

// 处理添加好友
bool OpeDB::handleAddFriend(const char *perUsrName, const char *myName)
{
    if (!perUsrName || !myName) return false;
    QString command = QString("INSERT INTO friendInfo (name, fname) VALUES ('%1', '%2')")
                          .arg(perUsrName).arg(myName);
    qDebug() << "(OpeDB::handleAddFriend) 执行SQL:" << command;
    QSqlQuery query;
    bool ret = query.exec(command);
    if (!ret) {
        qDebug() << "(OpeDB::handleAddFriend) 添加好友失败:" << query.lastError().text();
    }
    return ret;
}

// 刷新好友列表
QMap<QString, QString> OpeDB::handleRefFriend(const char *name)
{
    QMap<QString, QString> friendMap;
    if (!name) return friendMap;
    QString commandFriName = QString("SELECT name FROM friendInfo WHERE fname='%1' "
                                     "UNION SELECT fname FROM friendInfo WHERE name='%1'")
                                 .arg(name);
    QSqlQuery queryFriName;
    if (!queryFriName.exec(commandFriName)) {
        qDebug() << "(OpeDB::handleRefFriend) 查询好友失败:" << queryFriName.lastError().text();
        return friendMap;
    }
    while (queryFriName.next()) {
        QString friendName = queryFriName.value(0).toString();
        QString commandFriInfo = QString("SELECT online FROM usrInfo WHERE name='%1'")
                                     .arg(friendName);
        QSqlQuery queryFriInfo;
        if (queryFriInfo.exec(commandFriInfo) && queryFriInfo.next()) {
            friendMap.insert(friendName, queryFriInfo.value(0).toString());
        } else {
            qDebug() << "(OpeDB::handleRefFriend) 查询好友状态失败:" << queryFriInfo.lastError().text();
        }
    }
    return friendMap;
}

// 处理删除好友
bool OpeDB::handleDelFriend(const char *perUsrName, const char *myName)
{
    if (!perUsrName || !myName) return false;
    QString command = QString("DELETE FROM friendInfo WHERE "
                              "(name='%1' AND fname='%2') OR (name='%2' AND fname='%1')")
                          .arg(perUsrName).arg(myName);
    qDebug() << "(OpeDB::handleDelFriend) 执行SQL:" << command;
    QSqlQuery query;
    bool ret = query.exec(command);
    if (!ret) {
        qDebug() << "(OpeDB::handleDelFriend) 删除好友失败:" << query.lastError().text();
    }
    return ret;
}
