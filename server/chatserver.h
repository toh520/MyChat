#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <QObject>
#include <QList>
#include <QMap>

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

private:
    void processMessage(QTcpSocket *clientSocket, const QJsonObject &json);//各类消息的处理中枢
    void processLoginRequest(QTcpSocket *clientSocket,const QJsonObject &json);//登录请求
    void sendMessage(QTcpSocket *clientSocket,QJsonObject &json);//发送json消息给客户端
    void processChatMessage(QTcpSocket *senderSocket,const QJsonObject &json);//专门处理群聊聊天信息
    void processPrivateMessage(QTcpSocket *senderSocket,const QJsonObject &json);//私聊
    void broadcastUserList();//收集当前所有用户名并广播出去
    void loadUsers();//json文件里加载用户的函数



signals:
};

#endif // CHATSERVER_H
