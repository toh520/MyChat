#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <QObject>
#include <QList>

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


    void processMessage(QTcpSocket *clientSocket, const QJsonObject &json);//消息处理中枢
    void processLoginRequest(QTcpSocket *clientSocket,const QJsonObject &json);//登录请求
    void sendMessage(QTcpSocket *clientSocket,QJsonObject &json);//发送json消息给客户端



signals:
};

#endif // CHATSERVER_H
