#include "chatserver.h"
#include <QTcpServer> // 包含完整的QTcpServer类定义
#include <QTcpSocket>
#include <QDebug>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError> //捕获json解析错误
#include <QJsonArray>
#include <QDataStream>

#include <QFile>
#include <QDir>
#include <QDateTime>

ChatServer::ChatServer(QObject *parent)
    : QObject{parent}
{
    loadUsers();//读用户数据

    //创建聊天记录文件用于保存聊天记录
    QDir logDir("chat_logs");
    if(!logDir.exists()){
        qInfo()<<"不存在chat_log文档文件，自动创建";
        logDir.mkpath(".");//当前目录下创建
    }

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

    QString account=socketUserMap.value(clientSocket);
    //移除退出的用户
    socketUserMap.remove(clientSocket);
    if(!account.isEmpty()){
        userSocketMap.remove(account);
    }
    clientConnections.removeOne(clientSocket);
    socketIncompleteMessageSizeMap.remove(clientSocket);


    // 内存回收
    // 安全地删除socket对象
    // deleteLater()会等到事件循环空闲时再删除对象，这是处理Qt对象最安全的方式
    // 它可以防止在槽函数执行期间就删掉对象本身而引发的崩溃
    clientSocket->deleteLater();

    qInfo()<<"一个客户端退出，当前人数为："<<clientConnections.count();

    // 用户下线了，向剩下的人广播最新的用户列表
    broadcastUserList();
}

void ChatServer::handleReadyRead()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if(!clientSocket){
        return;
    }
    /*
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
    */

    //  解决粘包问题

    // 获取当前socket上一次未读完的消息大小，默认为0
    qint32 &incompleteMessageSize = socketIncompleteMessageSizeMap[clientSocket];

    QDataStream in(clientSocket);
    in.setVersion(QDataStream::Qt_5_12);

    for (;;) { // 使用循环来处理可能粘在一起的多个包
        // 1. 如果不知道包的长度，先尝试读取4字节的长度头
        if (incompleteMessageSize == 0) {
            if (clientSocket->bytesAvailable() < (int)sizeof(qint32)) {
                return; // 数据不够，等待下一次readyRead
            }
            in >> incompleteMessageSize;
        }

        // 2. 如果知道了长度，但消息体不完整
        if (clientSocket->bytesAvailable() < incompleteMessageSize) {
            return; // 消息体不完整，等待下一次readyRead
        }

        // 3. 读取完整的消息体
        QByteArray messageData;
        messageData.resize(incompleteMessageSize);
        in.readRawData(messageData.data(), incompleteMessageSize);

        // --- 到这里，messageData里就是一个完整的、干净的JSON了 ---

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(messageData, &parseError);

        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            // 解析成功，调用消息处理中枢
            processMessage(clientSocket, doc.object());
        } else {
            qWarning() << "服务器解析JSON失败:" << parseError.errorString() << "原始数据:" << messageData;
        }

        // 5. 一条消息处理完毕，重置大小，准备处理下一条（如果缓冲区里还有的话）
        incompleteMessageSize = 0;

    }

}

void ChatServer::processMessage(QTcpSocket *clientSocket, const QJsonObject &json)
{
    if(json.contains("type") && json["type"].isString()){//确保有type字段，判断消息类型
        QString type = json["type"].toString();
        if(type == "login"){
            processLoginRequest(clientSocket,json);//处理登录请求
        }else if(type=="chat_message"){
            //qInfo()<<"收到一条聊天信息："<<json["text"].toString();
            processChatMessage(clientSocket,json);;
        }else if(type=="private_message"){
            processPrivateMessage(clientSocket,json);
        }else if(type=="request_history"){
            processHistoryRequest(clientSocket,json);
        }else if(type == "report_udp_port"){
            processUdpPortReport(clientSocket,json);
        }else if (type == "request_call") {
            processCallRequest(clientSocket, json);
        }else if(type == "hangup_call"){
            processHangupCall(clientSocket, json);
        }else if (type == "accept_call") {
            processAcceptCall(clientSocket, json);
        }
        else if (type == "reject_call") {
            processRejectCall(clientSocket, json);
        }else if(type == "voice_message"){
            processVoiceMessage(clientSocket,json);
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


    //确定的用户名登录
    bool success = false;// 默认登录失败

    if(registeredUsers.contains(account)){//账号存在
        if(registeredUsers.value(account)==password){//密码正确
            success = true;
        }
    }

    QJsonObject response;
    response["type"]="login_response";
    response["success"]=success;


    if(success){
        qInfo()<<"账号："<<account<<"登录成功";

        //关联账号与socket
        socketUserMap.insert(clientSocket,account);
        userSocketMap.insert(account,clientSocket);

        response["message"]="登录成功，欢迎";

        //解决登录后没有用户列表
        // QList<QString> otherUsers;
        // for(auto it = socketUserMap.constBegin();it !=socketUserMap.constEnd();++it){
        //     if(it.key() != clientSocket){
        //         otherUsers.append(it.value());
        //     }
        // }

        // 将登录的用户加到列表里
        QList<QString> allUsers = socketUserMap.values();

        QJsonArray usersArray;
        for(const QString & user:allUsers){
            usersArray.append(user);
        }

        response["users"]=usersArray;

        sendMessage(clientSocket,response);//告诉当前用户登录成功了

        //广播给所有用户，用户列表更新
        broadcastUserList();


    }else{
        qWarning() << "登录失败：用户名" << account << "或密码错误。";
        response["message"]="登录失败，账号或密码错误";
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

    //【4字节头部 + 消息体】，解决多json发送时无法解析的问题
    //获取消息体的实际长度
    qint32 messageLength=data.size();
    //创建一个数据流，先写入4字节的长度，再写入消息体
    QByteArray block;
    QDataStream out(&block,QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_12);// 确保版本一致
    out<<messageLength;// 写入4字节的头部
    out.writeRawData(data.constData(),data.size());// 写入消息体

    //发送这个包含了头部和消息体的完整数据块
    clientSocket->write(block);//send


}

void ChatServer::processChatMessage(QTcpSocket *senderSocket, const QJsonObject &json)
{
    qInfo()<<"收到一条消息，准备广播";
    QString text=json["text"].toString();

    QJsonObject broadcastJson;
    broadcastJson["type"]="new_chat_message";
    broadcastJson["text"]=text;

    QString senderName=socketUserMap.value(senderSocket,"未知用户");
    broadcastJson["sender"]=senderName;
    // 暂时用IP:Port作为发送者标识
    //broadcastJson["sender"]=senderSocket->peerAddress().toString()+":"+QString::number(senderSocket->peerPort());


    //对所有用户广播
    for(QTcpSocket *otherSocket:clientConnections){
        sendMessage(otherSocket,broadcastJson);
    }

    QJsonObject logMessage = broadcastJson;
    logMessage["timestamp"]=QDateTime::currentDateTime().toString(Qt::ISODate);//设置时间

    // 调用之前创建的记录员函数，将消息存入世界频道日志
    appendMessageToLog("chat_logs/world_channel.json", logMessage);

}

void ChatServer::processHistoryRequest(QTcpSocket *clientSocket, const QJsonObject &json)
{
    QString channel = json["channel"].toString();
    QString logFileName;

    if(channel=="world_channel"){//世界频道
        logFileName="chat_logs/world_channel.json";
    }else{//私聊
        QString selfName = socketUserMap.value(clientSocket);
        if(!selfName.isEmpty()){
            logFileName = getPrivateChatLogFileName(selfName, channel);

        }else {
            qWarning() << "一个未登录的用户正在请求历史记录!";
            return;
        }
    }

    QFile logFile(logFileName);
    QJsonArray fullHistoryArray;

    // 读取日志文件，并解析成 JSON 数组
    if(logFile.open(QIODevice::ReadOnly | QIODevice::Text)){
        QByteArray data = logFile.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if(doc.isArray()){
            fullHistoryArray=doc.array();
        }
        logFile.close();
    }else{
        qInfo() << "历史记录文件不存在，无需发送:" << logFileName;
    }

    //截取最后MAX_HISTORY_MESSAGES条消息
    QJsonArray limitedHistoryArray;
    int startIndex = qMax(0,fullHistoryArray.size()-MAX_HISTORY_MESSAGES);

    for (int i = startIndex; i < fullHistoryArray.size(); ++i) {
        limitedHistoryArray.append(fullHistoryArray.at(i));
    }

    QJsonObject response;
    response["type"] = "history_response";
    response["channel"] = channel;
    response["history"] = limitedHistoryArray;

    qInfo() << "向客户端发送" << channel << "的" << limitedHistoryArray.count() << "条历史消息";
    sendMessage(clientSocket, response);
}

void ChatServer::processPrivateMessage(QTcpSocket *senderSocket, const QJsonObject &json)
{
    QString recipientName = json["recipient"].toString();
    QString text = json["text"].toString();
    QString senderName = socketUserMap.value(senderSocket, "未知用户");
    //获取接收者的socket
    QTcpSocket *recipientSocket = userSocketMap.value(recipientName,nullptr);

    QJsonObject privateMessageJson;
    privateMessageJson["type"] = "new_private_message"; // 使用新的类型，以便客户端区分
    privateMessageJson["sender"] = senderName;
    privateMessageJson["text"] = text;

    if(recipientSocket){
        sendMessage(recipientSocket,privateMessageJson);
        qInfo() << "私聊消息从" << senderName << "成功转发给" << recipientName;
    }else{
        //没有找到接收方的socket
        qWarning() << senderName << "尝试向离线用户" << recipientName << "发送消息。";
    }

    // //自己窗口也要显示消息
    // sendMessage(senderSocket,privateMessageJson);

    //存聊天记录
    QJsonObject logMessage = privateMessageJson;
    logMessage["recipient"]=recipientName;
    logMessage["timestamp"]=QDateTime::currentDateTime().toString(Qt::ISODate);
    //获取对应的文件名(统一标准)
    QString logFileName=getPrivateChatLogFileName(senderName,recipientName);
    // 调用通用的记录员函数，将消息存入专属档案
    appendMessageToLog(logFileName, logMessage);

}

void ChatServer::processUdpPortReport(QTcpSocket *clientSocket, const QJsonObject &json)
{
    QString username = socketUserMap.value(clientSocket);
    if(username.isEmpty()){
        qWarning() << "一个未登录的客户端尝试报告UDP端口。";
        return;
    }

    // 从消息中提取UDP端口号
    quint16 udpPort = static_cast<quint16>(json["port"].toInt());
    if (udpPort == 0) {
        qWarning() << "客户端" << username << "报告了一个无效的UDP端口: 0";
        return;
    }

    // 获取客户端的IP地址
    // 对于版本一（局域网），直接从TCP socket获取的IP地址
    QString ipAddress = clientSocket->peerAddress().toString();

    // 对于IPv6地址，它可能包含 "::ffff:" 前缀，我们把它清理掉
    if(ipAddress.startsWith("::ffff:")){
        ipAddress = ipAddress.mid(7);//从索引为 7 的位置开始的子字符串（包含索引 7 的字符及之后的所有字符）
    }

    //将 {IP, Port} 存入
    userUdpAddressMap[username]=qMakePair(ipAddress,udpPort);
    qInfo() << "已记录用户" << username << "的UDP地址:" << ipAddress << ":" << udpPort;


}

void ChatServer::processCallRequest(QTcpSocket *clientSocket, const QJsonObject &json)
{
    // 识别发起方和接收方
    QString callerName = socketUserMap.value(clientSocket);
    QString recipientName = json["recipient"].toString();

    if (callerName.isEmpty() || recipientName.isEmpty()){
        return;
    }

    qInfo() << "收到来自" << callerName << "发往" << recipientName << "的通话请求。";

    // 查找接收方的TCP Socket
    QTcpSocket *recipientSocket = userSocketMap.value(recipientName);

    // 检查接收方是否在线且地址已记录
    if (recipientSocket && userUdpAddressMap.contains(recipientName) && userUdpAddressMap.contains(callerName)){
        // 获取双方的UDP地址
        QPair<QString, quint16> callerAddr = userUdpAddressMap.value(callerName);
        QPair<QString, quint16> recipientAddr = userUdpAddressMap.value(recipientName);

        // 向发起方(caller)发送接收方(recipient)的地址
        QJsonObject responseToCaller;
        responseToCaller["type"] = "call_response";
        responseToCaller["peer_name"] = recipientName;
        responseToCaller["peer_ip"] = recipientAddr.first;
        responseToCaller["peer_port"] = recipientAddr.second;
        sendMessage(clientSocket, responseToCaller);


        // 向接收方(recipient)发送发起方(caller)的地址 (作为来电通知)
        QJsonObject offerToRecipient;
        offerToRecipient["type"] = "call_offer";
        offerToRecipient["peer_name"] = callerName;
        offerToRecipient["peer_ip"] = callerAddr.first;
        offerToRecipient["peer_port"] = callerAddr.second;
        sendMessage(recipientSocket, offerToRecipient);

        qInfo() << "地址交换成功：" << recipientName << "与" << callerName;

    }else {
        qWarning() << "地址交换失败：接收方" << recipientName << "不在线或未报告UDP地址。";

    }

}

void ChatServer::processHangupCall(QTcpSocket *clientSocket, const QJsonObject &json)
{
    QString senderName = socketUserMap.value(clientSocket);
    QString recipientName = json["recipient"].toString();

    qInfo() << senderName << "请求挂断与" << recipientName << "的通话。";

    QTcpSocket *recipientSocket =  userSocketMap.value(recipientName);

    if(recipientSocket){
        QJsonObject hangupNotice;
        hangupNotice["type"] = "hangup_call";
        // 我们可以包含挂断方的信息，但对于客户端来说，知道类型就够了
        hangupNotice["sender"] = senderName;
        sendMessage(recipientSocket,hangupNotice);
    }
}

void ChatServer::processAcceptCall(QTcpSocket *clientSocket, const QJsonObject &json)
{
    QString senderName = socketUserMap.value(clientSocket);
    QString recipientName = json["recipient"].toString();

    qInfo() << senderName << "接受了来自" << recipientName << "的通话请求。";

    QTcpSocket *recipientSocket = userSocketMap.value(recipientName);

    if (recipientSocket) {
        QJsonObject notice;
        notice["type"] = "accept_call";
        notice["sender"] = senderName;
        sendMessage(recipientSocket, notice);
    }
}

void ChatServer::processRejectCall(QTcpSocket *clientSocket, const QJsonObject &json)
{
    QString senderName = socketUserMap.value(clientSocket);
    QString recipientName = json["recipient"].toString();

    qInfo() << senderName << "拒绝了来自" << recipientName << "的通话请求。";

    QTcpSocket *recipientSocket = userSocketMap.value(recipientName);
    if (recipientSocket) {
        QJsonObject notice;
        notice["type"] = "reject_call";
        notice["sender"] = senderName;
        sendMessage(recipientSocket, notice);
    }
}

void ChatServer::processVoiceMessage(QTcpSocket *clientSocket, const QJsonObject &json)
{
    QString senderName = socketUserMap.value(clientSocket, "未知用户");
    QString recipientName = json["recipient"].toString();

    qInfo() << "收到来自 " << senderName << " 发给 " << recipientName << " 的语音消息，准备转发。";

    // 构建要转发给接收者的消息
    QJsonObject forwardJson;
    forwardJson["type"] = "new_voice_message"; // 使用新类型，以便客户端区分
    forwardJson["sender"] = senderName;
    forwardJson["duration_ms"] = json["duration_ms"].toInt();
    forwardJson["format"] = json["format"].toString();
    forwardJson["data"] = json["data"].toString();
    forwardJson["channel"] = recipientName; // "世界频道" 或 私聊对象名

    // 判断是私聊还是群聊
    if (recipientName == "世界频道") {
        // 广播给除了自己以外的所有人
        for (QTcpSocket *otherSocket : clientConnections) {
            if (otherSocket != clientSocket) {
                sendMessage(otherSocket, forwardJson);
            }
        }
        qInfo() << "已将语音消息广播到世界频道。";
    } else {//私聊
        QTcpSocket *recipientSocket = userSocketMap.value(recipientName, nullptr);
        if (recipientSocket) {
            sendMessage(recipientSocket, forwardJson);
            qInfo() << "已将语音消息成功转发给 " << recipientName;
        } else {
            qWarning() << senderName << " 尝试向离线用户 " << recipientName << " 发送语音消息。";
        }

    }

}

void ChatServer::broadcastUserList()
{
    qInfo()<<"正在向所有客户端广播最新的用户列表...";

    QList<QString> userList=socketUserMap.values();//获取所有账号

    QJsonObject userListJson;
    userListJson["type"]="user_list_update";
    //存入用户列表
    QJsonArray usersArray;
    for(const QString &user:userList){
        usersArray.append(user);
    }
    userListJson["users"]=usersArray;

    for(QTcpSocket *client:clientConnections){
        sendMessage(client,userListJson);
    }
}

void ChatServer::loadUsers()
{
    QFile usersFile("users.json");
    if (!usersFile.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开 users.json 文件！请确保它和服务器程序在同一个目录下。";
        return;
    }

    QByteArray fileData = usersFile.readAll();
    usersFile.close();

    QJsonDocument doc = QJsonDocument::fromJson(fileData);
    if(!doc.isArray()){
        qWarning() << "users.json 的格式不正确，根元素必须是一个数组";
        return;
    }

    //遍历账号密码
    QJsonArray usersArray = doc.array();
    for(const QJsonValue &userValue:usersArray){
        QJsonObject userObject = userValue.toObject();
        QString username = userObject["username"].toString();
        QString password = userObject["password"].toString();
        //存入
        if (!username.isEmpty() && !password.isEmpty()) {
            registeredUsers.insert(username, password);
        }
    }
    qInfo() << "成功加载了" << registeredUsers.count() << "个用户账户";

}

QString ChatServer::getPrivateChatLogFileName(const QString &user1, const QString &user2) const
{
    //为了保证文件不冲突，用统一的文件命名方式
    // 通过按字母顺序排序用户名,确保文件名始终一致，无论谁是发送者
    QString sortedUser1 = user1;
    QString sortedUser2 = user2;

    if (sortedUser1 > sortedUser2) {
        qSwap(sortedUser1, sortedUser2);
    }

    return QString("chat_logs/private_%1_%2.json").arg(sortedUser1).arg(sortedUser2);
}

void ChatServer::appendMessageToLog(const QString &logFileName, const QJsonObject &messageObject)
{
    //工作流程是：读取现有的日志文件 -> 解析为JSON数组 -> 将新消息添加到数组末尾 -> 将更新后的数组写回文件
    QFile logFile(logFileName);

    //读取现有的日志文件
    QJsonArray logArray;
    if(logFile.open(QIODevice::ReadOnly | QIODevice::Text)){
        QByteArray data = logFile.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if(doc.isArray()){
            logArray = doc.array();
        }
        logFile.close();
    }

    //将新消息添加到数组末尾
    logArray.append(messageObject);

    // 将更新后的数组写回文件,WriteOnly会在文件不存在时创建
    if(logFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)){//覆盖写
        QJsonDocument newDoc(logArray);
        logFile.write(newDoc.toJson());
        logFile.close();
    }else{
        qWarning() << "无法写入聊天记录文件:" << logFileName;
    }



}
