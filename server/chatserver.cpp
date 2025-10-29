#include "chatserver.h"
#include <QTcpServer> // 包含完整的QTcpServer类定义
#include <QTcpSocket>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError> //捕获json解析错误

ChatServer::ChatServer(QObject *parent)
    : QObject{parent}
{
    tcpServer = new QTcpServer(this);
}

void ChatServer::startServer(quint16 port)
{
    // 1. 设置信号槽连接：当总机有新连接时，调用我们的处理函数
    // 这就是Qt异步编程的核心：我们告诉它“有事了叫我”，而不是傻等
    connect(tcpServer,&QTcpServer::newConnection,this,&ChatServer::handleNewConnection);
    // connect(
    //     tcpServer,               // 第一个参数：信号的发送者（谁会触发事件）
    //     &QTcpServer::newConnection,  // 第二个参数：要监听的信号（触发什么事件）
    //     this,                    // 第三个参数：信号的接收者（谁来处理事件）
    //     &ChatServer::handleNewConnection  // 第四个参数：处理事件的槽函数（怎么处理）
    // );

    if(!tcpServer->listen(QHostAddress::Any,port)){//监听失败
        qCritical()<<"服务器启动失败："<<tcpServer->errorString();
        return;
    }

    //监听成功
    qInfo()<<"服务器在端口："<<port<<"启动，等待连接";

}

void ChatServer::handleNewConnection()
{
    qInfo()<<"检测到有新客户的连接请求";

    QTcpSocket *clientSocket = tcpServer->nextPendingConnection();//类似于accpect

    //连接失败
    if(!clientSocket){
        qWarning()<<"无法获取客户端连接";
        return;
    }

    //成功,加入列表
    clientConnections.append(clientSocket);
    qInfo()<<"一个客户端已经连接，当前在线人数："<<clientConnections.count();


    //客户断开连接
    connect(clientSocket,&QTcpSocket::disconnected,this,&ChatServer::handleClientDisconnected);

    //客户发送数据,用readyread解决recv的阻塞问题，只在readyRead()信号被触发时，才去调用handleReadyRead()函数。在这个函数里，我们才使用readAll()去读取数据。这意味着，我们永远只在确定有数据可读的时候才去读
    connect(clientSocket,&QTcpSocket::readyRead,this,&ChatServer::handleReadyRead);





}

void ChatServer::handleClientDisconnected()
{
    // 获取发射信号的对象
    // sender()返回一个QObject指针，它指向那个发射了当前正在处理的信号的对象
    // 在这里，它就是那个断开连接的QTcpSocket
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());//知道是谁发出的信号

    //防止sender返回的不是socket
    if(!clientSocket){
        return;
    }

    //移除退出的用户
    clientConnections.removeOne(clientSocket);
    qInfo()<<"一个客户端退出，当前人数为："<<clientConnections.count();

    // 内存回收
    // 安全地删除socket对象
    // deleteLater()会等到事件循环空闲时再删除对象，这是处理Qt对象最安全的方式
    // 它可以防止在槽函数执行期间就删掉对象本身而引发的崩溃
    clientSocket->deleteLater();


}

void ChatServer::handleReadyRead()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if(!clientSocket){
        return;
    }

    //获取所有数据
    QByteArray data = clientSocket->readAll();

    //qInfo()<<"收到来自客户端:"<<clientSocket->peerAddress().toString()<<"的消息:"<<data;

    //解析json
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data,&parseError);

    if(parseError.error != QJsonParseError::NoError){
        qWarning()<<"json解析失败："<<parseError.errorString()<<"原始数据："<<data;
        return;
    }

    if(!doc.isObject()){//不是json对象
        qWarning()<<"收到的不是json对象";
        return;

    }

    //转换为json
    QJsonObject jsonObj = doc.object();
    processMessage(clientSocket,jsonObj);






}

void ChatServer::processMessage(QTcpSocket *clientSocket, const QJsonObject &json)
{
    if(json.contains("type") && json["type"].isString()){//确保有type字段，判断消息类型
        QString type = json["type"].toString();
        if(type == "login"){
            processLoginRequest(clientSocket,json);//处理登录请求
        }else{
            qWarning()<<"收到未知类型的消息"<<type;
        }
    }else{
        qWarning()<<"收到的JSON缺少'type'字段。";
    }

}

void ChatServer::processLoginRequest(QTcpSocket *clientSocket, const QJsonObject &json)
{
    qInfo()<<"收到一个登录请求";

    QString account = json["account"].toString();
    QString password = json["password"].toString();

    //登录验证
    QJsonObject response;
    response["type"]="login_response";

    if(password=="123456"){
        qInfo()<<"账号："<<account<<"登录成功";
        response["success"]=true;
        response["message"]="登录成功，欢迎";

    }else{
        qWarning()<<"账号："<<account<<"登录失败，密码错误";
        response["success"]=false;
        response["message"]="登录失败，密码错误";
    }

    sendMessage(clientSocket,response);//发送消息回客户端


}

void ChatServer::sendMessage(QTcpSocket *clientSocket, QJsonObject &json)
{
    if(!clientSocket||!clientSocket->isOpen()){
        qWarning()<<"尝试向一个无效或已关闭的socket发送消息";
        return ;
    }
    QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);//将JSON对象转换为用于网络传输的QByteArray
    qInfo()<<"向客户端"<<clientSocket->peerAddress().toString()<<"发送消息："<<data;

    clientSocket->write(data);//send


}
