#include "chatwindow.h"
#include "ui_chatwindow.h"
#include <QDebug>
#include <QJsonArray>
//#include <QDataStream>

#include <QTextBrowser>
#include <QVBoxLayout>

ChatWindow::ChatWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ChatWindow)
{
    ui->setupUi(this);
}

ChatWindow::ChatWindow(QTcpSocket *Socket,const QJsonArray &initialUsers,const QString &username, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ChatWindow)
{
    ui->setupUi(this);
    this->socket = Socket;
    this->myUsername=username;
    // 关键一步：将socket的所有权转移到这个新窗口
    // 这样，当ChatWindow关闭时，socket也会被安全地销毁
    // 同时也断开了与原先MainWindow的父子关系

    // 1. 断开这个socket与之前所有对象的所有连接！
    //    这确保了MainWindow的槽函数不会再被触发。
    this->socket->disconnect();
    // 2. 将socket的父对象设置为当前窗口，确保内存被正确管理。
    this->socket->setParent(this);



    //获取用户列表
    ui->userListWidget->clear();
    for(const QJsonValue &user:initialUsers){
        ui->userListWidget->addItem(user.toString());
    }

    currentPrivateChatUser = ""; // 初始化为空，表示当前是群聊模式

    //将私聊与群聊页面分开，创建群聊
    QWidget *worldChannelTab = new QWidget();
    QTextBrowser *worldChannelBrowser = new QTextBrowser();
    //布局管理器
    QVBoxLayout *tabLayout = new QVBoxLayout(worldChannelTab);
    tabLayout->setContentsMargins(0,0,0,0);// 让布局紧贴房间边缘，不留白
    tabLayout->addWidget(worldChannelBrowser); // 把“布告栏”放进布局里

    //加入界面中
    ui->chatTabWidget->addTab(worldChannelTab,"世界频道");

    sessionBrowsers.insert("world_channel",worldChannelBrowser);

    //加载历史聊天记录
    QJsonObject historyRequest;
    historyRequest["type"] = "request_history";
    historyRequest["channel"] = "world_channel"; // 表明想要的是世界频道的历史
    // //告诉服务器
    // QByteArray dataToSend = QJsonDocument(historyRequest).toJson(QJsonDocument::Compact);
    // Socket->write(dataToSend);
    sendMessage(historyRequest);

    // 初始话udp
    udpSocket = new QUdpSocket(this);

    //绑定到一个随机的可用端口
    if(udpSocket->bind(QHostAddress::Any,0)){//通过 bind(QHostAddress::Any, 0) 来让操作系统为我们选择一个端口
        quint16 udpPort = udpSocket->localPort();//获取端口
        qDebug() << "UDP Socket 成功绑定到端口:" << udpPort;

        // 将这个端口号报告给服务器
        QJsonObject udpReport;
        udpReport["type"] = "report_udp_port";
        udpReport["port"] = udpPort;

        sendMessage(udpReport);
        qDebug() << "已将UDP端口号报告给服务器。";

    }else{
        qWarning() << "UDP Socket 绑定失败!";
    }



    //信号与槽
    connect(socket,&QTcpSocket::readyRead,this,&ChatWindow::onSocketReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &ChatWindow::onSocketDisconnected);// 我们也需要处理断开连接的情况，以防服务器中途关闭
    connect(udpSocket,&QUdpSocket::readyRead,this,&ChatWindow::onUdpSocketReadyRead);

}

ChatWindow::~ChatWindow()
{
    delete ui;
}

void ChatWindow::on_sendButton_clicked()
{
    QString text=ui->messageLineEdit->text();

    if(text.isEmpty()){//空消息
        return;
    }

    //创建消息发送的json
    QJsonObject messageObject;

    //区分私聊或者群聊,根据标签选择发送对象
    int currentIndex = ui->chatTabWidget->currentIndex();
    QString id= ui->chatTabWidget->tabText(currentIndex);

    if(id=="世界频道"){
        //群里
        messageObject["type"]="chat_message";
        messageObject["text"]=text;
    }else{
        //私聊
        messageObject["type"]="private_message";
        messageObject["recipient"] = id; // 指定接收者
        messageObject["text"] = text;
    }

    // if(currentPrivateChatUser.isEmpty()){
    //     //群里
    //     messageObject["type"]="chat_message";
    //     messageObject["text"]=text;
    // }else{
    //     //私聊
    //     messageObject["type"]="private_message";
    //     messageObject["recipient"] = currentPrivateChatUser; // 指定接收者
    //     messageObject["text"] = text;
    // }


    // //json转换为qbytearray用来网络传输
    // QByteArray dataToSend = QJsonDocument(messageObject).toJson(QJsonDocument::Compact);//参数QJsonDocument::Compact是一个枚举值，指定转换后的 JSON 格式为紧凑模式（即去除多余的空格、换行，生成一行紧凑的字符串），适合网络传输（减少数据量）或存储。

    // socket->write(dataToSend);//send
    //qDebug()<<"发送聊天消息："<<dataToSend;

    sendMessage(messageObject);//解决json粘包

    QTextBrowser *currentBrowser = sessionBrowsers.value(id);
    if(currentBrowser){
        currentBrowser->append(QString("<font color='green'>[我]:</font> %1").arg(text));
    }

    //清空输入栏
    ui->messageLineEdit->clear();
    ui->messageLineEdit->setFocus();

}

void ChatWindow::onSocketReadyRead()
{
    QDataStream in(socket);
    in.setVersion(QDataStream::Qt_5_12);

    for (;;) { // 使用循环来处理可能粘在一起的多个包
        // 1. 处理半包：如果上一次没收全头部
        if (incompleteMessageSize == 0) {
            // 检查缓冲区里的数据是否足够读取一个完整的头部（4字节）
            if (socket->bytesAvailable() < (int)sizeof(qint32)) {
                return; // 数据不够，等待下一次readyRead
            }
            // 读取头部，得到即将到来的消息体长度
            in >> incompleteMessageSize;
        }

        // 2. 处理半包：如果已经知道了长度，但消息体没收全
        if (socket->bytesAvailable() < incompleteMessageSize) {
            return; // 消息体不完整，等待下一次readyRead
        }

        // 3. 读取完整的消息体
        QByteArray messageData;
        messageData.resize(incompleteMessageSize);
        in.readRawData(messageData.data(), incompleteMessageSize);

        // --- 到这里，messageData里就是一个完整的、干净的JSON了 ---

        QJsonDocument doc = QJsonDocument::fromJson(messageData);
        if (doc.isObject()) {
            QJsonObject jsonObj = doc.object();
            QString type = jsonObj["type"].toString();

            if (type == "new_chat_message") {
                QString sender = jsonObj["sender"].toString();
                QString text = jsonObj["text"].toString();
                //找到对应的世界频道
                QTextBrowser *browser= sessionBrowsers.value("world_channel");
                if(browser){
                    browser->append(QString("[世界][%1]: %2").arg(sender).arg(text));
                }
                //ui->messageBrowser->append(QString("[世界][%1]: %2").arg(sender).arg(text));
            } else if (type == "user_list_update") {
                QJsonArray usersArray = jsonObj["users"].toArray();
                ui->userListWidget->clear();
                for (const QJsonValue &user : usersArray) {
                    ui->userListWidget->addItem(user.toString());
                }
            }else if(type=="new_private_message"){
                QString sender = jsonObj["sender"].toString();
                QString text = jsonObj["text"].toString();
                //ui->messageBrowser->append(QString("<font color='blue'>[私聊] 来自 [%1]:</font> %2").arg(sender).arg(text));
                if(!sessionBrowsers.contains(sender)){
                    // 如果不存在，说明这是对方第一次发来私聊
                    // 我们需要自动为他创建一个新的标签页
                    QWidget *w = new QWidget();
                    QTextBrowser *t = new QTextBrowser();
                    QVBoxLayout *v = new QVBoxLayout(w);
                    v->setContentsMargins(0,0,0,0);
                    v->addWidget(t);
                    int i=ui->chatTabWidget->addTab(w,sender);
                    sessionBrowsers.insert(sender,t);
                    requestHistoryForChannel(sender);
                }

                QTextBrowser *browser = sessionBrowsers.value(sender);
                if(browser){
                    //填入消息内容
                    browser->append(QString("<font color='blue'>[私聊] 来自 %1:</font> %2").arg(sender).arg(text));
                }  
            }else if(type=="history_response"){
                QString channel = jsonObj["channel"].toString();
                QJsonArray history = jsonObj["history"].toArray();

                QTextBrowser *browser=sessionBrowsers.value(channel);
                if(browser){
                    // 先清空，再加载历史记录
                    browser->clear();

                    // 将历史消息逐条插入到显示窗口的顶部
                    for(const QJsonValue &msgValue:history){
                        QJsonObject msgObj = msgValue.toObject();
                        QString sender = msgObj["sender"].toString();
                        QString text = msgObj["text"].toString();
                        QString timestamp = msgObj["timestamp"].toString(); // 服务器记录的时间戳

                        // 为了区分历史消息，我们可以用不同的颜色或格式
                        QString formattedMessage;
                        if (sender == myUsername) { // 如果是自己发的消息
                            formattedMessage = QString("<font color='gray'>[%1]</font> <font color='green'>[我]:</font> <font color='gray'>%2</font>")
                                                   .arg(timestamp.left(19).replace("T", " "))
                                                   .arg(text);
                        } else { // 别人发的消息
                            formattedMessage = QString("<font color='gray'>[%1] [%2]: %3</font>")
                                                   .arg(timestamp.left(19).replace("T", " "))
                                                   .arg(sender)
                                                   .arg(text);
                        }
                        browser->insertHtml(formattedMessage+"<br>");

                    }

                    // 在历史记录加载完后，显示一条的分割线
                    if (!history.isEmpty()) {
                        browser->append("<hr><em><p align='center' style='color:gray;'>--- 以上是历史消息 ---</p></em>");
                    }
                }
            }else if(type == "call_response" || type == "call_offer"){//通话模块
                QString peerName = jsonObj["peer_name"].toString();
                QString peerIp = jsonObj["peer_ip"].toString();
                quint16 peerPort = static_cast<quint16>(jsonObj["peer_port"].toInt());

                currentCallPeerAddress.setAddress(peerIp);
                currentCallPeerPort = peerPort;

                if (type == "call_response") {//作为发起方
                    qDebug() << "通话请求成功！获取到对方" << peerName << "的UDP地址:" << peerIp << ":" << peerPort;

                    //发送一个 "ping" 来测试UDP通道
                    QByteArray pingData = "ping";
                    udpSocket->writeDatagram(pingData,currentCallPeerAddress,currentCallPeerPort);//这是发送UDP数据的核心函数。它是一个“无连接”的发送，直接指定数据、目标地址和目标端口即可。它会立即返回，不会等待对方确认。

                    qDebug() << "已向对方发送UDP 'ping'。";
                } else { // type == "call_offer"，接收方
                    qDebug() << "收到来自" << peerName << "的来电！对方UDP地址:" << peerIp << ":" << peerPort;


                }


            }else{

            }
        }

        // 5. 一条消息处理完毕，重置incompleteMessageSize，准备处理下一条
        incompleteMessageSize = 0;
    }


    // QByteArray data = socket->readAll();
    // QJsonParseError parseError;
    // QJsonDocument doc = QJsonDocument::fromJson(data,&parseError);

    // //解析是否成功
    // if(parseError.error!=QJsonParseError::NoError || !doc.isObject()){
    //     qWarning() << "解析服务器消息失败:" << parseError.errorString();
    //     return;
    // }
    // QJsonObject jsonObj=doc.object();
    // QString type=jsonObj["type"].toString();
    // if(type=="new_chat_message"){
    //     QString sender = jsonObj["sender"].toString();
    //     QString text = jsonObj["text"].toString();
    //     //qDebug() << "客户端收到新消息 -> 来自:" << sender << "内容:" << text;

    //     //用QString::arg()来创建一个漂亮的格式，比如 "[发送者]: 消息内容"
    //     ui->messageBrowser->append(QString("[%1]:%2").arg(sender).arg(text));

    // }else if(type=="user_list_update"){
    //     QJsonArray userArray = jsonObj["users"].toArray();

    //     //更新列表
    //     ui->userListWidget->clear();
    //     for(const QJsonValue &user:userArray){// 遍历从服务器收到的用户数组，逐个添加到UI上
    //         ui->userListWidget->addItem(user.toString());
    //     }

    // }
}

void ChatWindow::onSocketDisconnected()
{
    qWarning() << "与服务器的连接已断开，聊天窗口将关闭。";
    this->close();
}

void ChatWindow::requestHistoryForChannel(const QString &channel)
{
    if (channel.isEmpty()) {
        return;
    }

    QJsonObject historyRequest;
    historyRequest["type"] = "request_history";
    historyRequest["channel"] = channel;

    sendMessage(historyRequest);

    qDebug() << "历史消息获取:" << channel;
}


void ChatWindow::on_userListWidget_itemDoubleClicked(QListWidgetItem *item)
{
    QString clickedUser = item->text();

    // 不能和自己聊天
    if (clickedUser == myUsername) {
        return; // 如果双击的是自己，则不做任何事
    }

    // 检查是否已经存在与该用户的私聊标签页
    if(sessionBrowsers.contains(clickedUser)){
        //存在即切换
        for(int i=0;i<ui->chatTabWidget->count();i++){
            if(ui->chatTabWidget->tabText(i)==clickedUser){
                ui->chatTabWidget->setCurrentIndex(i);
                return;
            }
        }
    }else{
        //不存在创建
        QWidget *privateTab= new QWidget();
        QTextBrowser *privateBrowser = new QTextBrowser();
        QVBoxLayout *tabLayout = new QVBoxLayout(privateTab);
        tabLayout->setContentsMargins(0,0,0,0);
        tabLayout->addWidget(privateBrowser);

        int newIndex= ui->chatTabWidget->addTab(privateTab,clickedUser);
        sessionBrowsers.insert(clickedUser,privateBrowser);

        // 当我们第一次创建私聊窗口时，请求这个私聊的历史记录
        requestHistoryForChannel(clickedUser);

        //切换到该页
        ui->chatTabWidget->setCurrentIndex(newIndex);
    }

    // // 如果双击的是当前已经选中的用户，则取消私聊，回到群聊模式
    // if(clickedUser==currentPrivateChatUser){
    //     currentPrivateChatUser="";
    //     ui->chatTargetLabel->setText("当前频道：世界频道");
    //     qDebug()<<"已退出私聊模式，回到世界频道";
    // }else{
    //     currentPrivateChatUser=clickedUser;
    //     ui->chatTargetLabel->setText(QString("正在与 [ %1 ] 私聊... (再次双击可退出)").arg(clickedUser));
    //     qDebug()<<"已进入与" << clickedUser << "的私聊模式。";
    // }


    //模拟一下通话，测试
    QJsonObject callRequest;
    callRequest["type"] = "request_call";
    callRequest["recipient"] = clickedUser; // 告诉服务器想和谁通话
    sendMessage(callRequest); // 使用我们统一的发送函数
    qDebug() << "向服务器发起与" << clickedUser << "的通话请求。";
}

void ChatWindow::sendMessage(const QJsonObject &message)
{
    if (!socket || !socket->isOpen()) {
        qWarning() << "尝试通过一个无效或已关闭的TCP socket发送消息。";
        return;
    }

    QByteArray dataToSend = QJsonDocument(message).toJson(QJsonDocument::Compact);
    // 【4字节头部 + 消息体】，解决粘包问题
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_12); // 确保版本一致

    // 写入4字节的头部（消息体长度）
    out << static_cast<qint32>(dataToSend.size());
    // 写入消息体
    out.writeRawData(dataToSend.constData(), dataToSend.size());

    // 发送这个包含了头部和消息体的完整数据块
    socket->write(block);
    qDebug() << "已发送json，包含头大小：" << dataToSend.size();

}

void ChatWindow::onUdpSocketReadyRead()
{
    // 只要socket里有数据，就一直循环读取
    while(udpSocket->hasPendingDatagrams()){
        // 创建一个足够大的缓冲区来接收数据
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());

        QHostAddress senderAddress;
        quint16 senderPort;

        // 读取一个数据包，同时获取发送方的地址和端口
        udpSocket->readDatagram(datagram.data(),datagram.size(),&senderAddress,&senderPort);

        // 在控制台打印收到的消息
        qDebug() << "收到来自" << senderAddress.toString() << ":" << senderPort << "的UDP消息:" << datagram;

        //我们一个 "pong" 回复
        if(datagram == "ping"){
            QByteArray pong = "pong";
            udpSocket->writeDatagram(pong,senderAddress,senderPort);
            qDebug() << "已向对方回复UDP 'pong'。";
        }
    }
}

