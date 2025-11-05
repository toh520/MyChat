#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QPair>



#define MAX_HISTORY_MESSAGES 10 // 用于限制返回的历史消息最大条数

class QTcpServer;
class QTcpSocket;

class ChatServer : public QObject
{
    Q_OBJECT
public:
    explicit ChatServer(QObject *parent = nullptr);
    void startServer(quint16 port);// quint16是专门用于表示端口号的无符号16位整数

private slots:
    void handleNewConnection();//处理连接
    void handleClientDisconnected();//处理断开
    void handleReadyRead();//处理消息发送（来自客户端）

private:
    QTcpServer *tcpServer;// 服务器接待总机
    QList<QTcpSocket*> clientConnections;//存放客户的列表
    QMap<QTcpSocket*,QString> socketUserMap;//记录 Socket -> 用户名 的映射:“键-值”映射
    QMap<QString,QTcpSocket*> userSocketMap;//反向查找用
    QMap<QString,QString> registeredUsers;//账号和他们的密码
    QMap<QString,QPair<QString,quint16>> userUdpAddressMap;// 用户名 -> {IP, UDP端口} 的映射

    QMap<QTcpSocket*, qint32> socketIncompleteMessageSizeMap; // 记录每个socket的不完整消息大小,解决粘包问题


private:
    void processMessage(QTcpSocket *clientSocket, const QJsonObject &json);//各类消息的处理中枢
    void processLoginRequest(QTcpSocket *clientSocket,const QJsonObject &json);//登录请求
    void sendMessage(QTcpSocket *clientSocket,QJsonObject &json);//发送json消息给客户端
    void processChatMessage(QTcpSocket *senderSocket,const QJsonObject &json);//专门处理群聊聊天信息
    void processHistoryRequest(QTcpSocket *clientSocket, const QJsonObject &json); // 处理历史记录请求
    void processPrivateMessage(QTcpSocket *senderSocket,const QJsonObject &json);//私聊
    void processUdpPortReport(QTcpSocket *clientSocket, const QJsonObject &json);//处理客户端报告UDP端口的请求
    void processCallRequest(QTcpSocket *clientSocket, const QJsonObject &json);// 处理通话请求

    void broadcastUserList();//收集当前所有用户名并广播出去
    void loadUsers();//json文件里加载用户的函数


    QString getPrivateChatLogFileName(const QString &user1, const QString &user2) const;// 获取私聊记录文件名
    void appendMessageToLog(const QString &logFileName, const QJsonObject &messageObject);//记录聊天记录


signals:
};

#endif // CHATSERVER_H
