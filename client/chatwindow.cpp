#include "chatwindow.h"
#include "callwindow.h"

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
    //音频格式定义
    audioFormat.setSampleRate(44100);// 设置采样率：44100Hz
    audioFormat.setChannelCount(1);// 设置声道数：1 (单声道)
    audioFormat.setSampleFormat(QAudioFormat::Int16);// 设置样本格式：16位整数

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

    //通话窗口
    // callWin = new callWindow();
    // callWin->hide(); // 默认是隐藏的

    //信号与槽
    connect(socket,&QTcpSocket::readyRead,this,&ChatWindow::onSocketReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &ChatWindow::onSocketDisconnected);// 我们也需要处理断开连接的情况，以防服务器中途关闭
    connect(udpSocket,&QUdpSocket::readyRead,this,&ChatWindow::onUdpSocketReadyRead);
    // connect(callWin,&callWindow::accepted,this,[=](){
    //     qDebug() << "用户点击了接听，向" << currentCallPeerName << "发送 accept_call 信令";

    //     // 发送 accept_call 信令
    //     QJsonObject acceptMsg;
    //     acceptMsg["type"] = "accept_call";
    //     acceptMsg["recipient"] = currentCallPeerName;
    //     sendMessage(acceptMsg);

    //     // 启动自己的音频
    //     startAudio(QMediaDevices::defaultAudioInput(), QMediaDevices::defaultAudioOutput());

    //     // 将通话窗口切换到“通话中”状态
    //     callWin->showInCall(currentCallPeerName);

    // });// 当用户点击接听时
    // connect(callWin, &callWindow::rejected, this,[this]{
    //     qDebug() << "用户点击了拒绝，向" << currentCallPeerName << "发送 reject_call 信令";

    //     QJsonObject rejectMsg;
    //     rejectMsg["type"] = "reject_call";
    //     rejectMsg["recipient"] = currentCallPeerName;
    //     sendMessage(rejectMsg);

    // });// 当用户点击拒绝时
    // connect(callWin,&callWindow::hangedUp,this,[this]{
    //     qDebug() << "用户点击了挂断，向" << currentCallPeerName << "发送 hangup_call 信令";

    //     // 发送 hangup_call 信令
    //     QJsonObject hangupMsg;
    //     hangupMsg["type"] = "hangup_call";
    //     hangupMsg["recipient"] = currentCallPeerName;
    //     sendMessage(hangupMsg);

    //     // 立刻停止本地音频
    //     stopAudio();
    // });// 当用户在通话中点击挂断时
}

ChatWindow::~ChatWindow()
{
    // // 如果窗口关闭时通话仍在进行, 确保清理
    // if (callWin) {
    //     callWin->close();
    // }
    delete ui;
    //delete callWin;
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
            }/*else if(type == "call_response"||type == "call_offer"){//通话模块
                QString peerName = jsonObj["peer_name"].toString();
                QString peerIp = jsonObj["peer_ip"].toString();
                quint16 peerPort = static_cast<quint16>(jsonObj["peer_port"].toInt());

                switchToOrOpenPrivateChat(peerName);

                currentCallPeerAddress.setAddress(peerIp);
                currentCallPeerPort = peerPort;

                // 解释：无论我是发起方(call_response)还是接收方(call_offer)，
                // 一旦地址交换成功，就应该启动音频设备。
                qInfo() << "通话已建立，正在启动音频设备...";
                startAudio(QMediaDevices::defaultAudioInput(),QMediaDevices::defaultAudioOutput());//返回操作系统当前设置的默认麦克风设备,默认扬声器/耳机设备

                ui->callButton->setVisible(false);   // 隐藏呼叫按钮
                ui->hangupButton->setVisible(true);  // 显示挂断按钮
                //

                if (type == "call_response") {//作为发起方
                    qDebug() << "通话请求成功！获取到对方" << peerName << "的UDP地址:" << peerIp << ":" << peerPort;

                    // //发送一个 "ping" 来测试UDP通道
                    // QByteArray pingData = "ping";
                    // udpSocket->writeDatagram(pingData,currentCallPeerAddress,currentCallPeerPort);//这是发送UDP数据的核心函数。它是一个“无连接”的发送，直接指定数据、目标地址和目标端口即可。它会立即返回，不会等待对方确认。

                    // qDebug() << "已向对方发送UDP 'ping'。";
                } else { // type == "call_offer"，接收方
                    qDebug() << "收到来自" << peerName << "的来电！对方UDP地址:" << peerIp << ":" << peerPort;

                }
            }*//*else if(type == "call_response"){//处理“呼叫成功”的响应（我是呼叫方）
                if (callWin) return; // 如果已经有通话窗口, 忽略, 防止重复创建

                QString peerName = jsonObj["peer_name"].toString();
                QString peerIp = jsonObj["peer_ip"].toString();
                quint16 peerPort = static_cast<quint16>(jsonObj["peer_port"].toInt());

                qDebug() << "通话请求成功！获取到对方" << peerName << "的UDP地址:" << peerIp << ":" << peerPort;
                currentCallPeerName = peerName;//记录下来电者

                // 启动音频设备
                currentCallPeerAddress.setAddress(peerIp);
                currentCallPeerPort = peerPort;
                startAudio(QMediaDevices::defaultAudioInput(), QMediaDevices::defaultAudioOutput());

                // *** 动态创建 callWindow ***
                callWin = new callWindow();
                callWin->setAttribute(Qt::WA_DeleteOnClose); // 窗口关闭时自动删除
                connect(callWin, &callWindow::hangedUp, this, &ChatWindow::onHangupClicked);
                connect(callWin, &QObject::destroyed, this, [this](){ callWin = nullptr; }); // <--- 安全措施

                // 弹出“通话中”窗口
                callWin->showInCall(peerName);
            }else if(type == "call_offer"){//处理“收到来电”的请求（我是接收方）
                if (callWin) return; // 如果正在通话中, 忽略新的来电

                QString peerName = jsonObj["peer_name"].toString();
                QString peerIp = jsonObj["peer_ip"].toString();
                quint16 peerPort = static_cast<quint16>(jsonObj["peer_port"].toInt());

                qDebug() << "收到来自" << peerName << "的来电！";

                // 确保第一时间记录来电者
                currentCallPeerName = peerName;

                // 暂存对方信息，等待用户接听
                currentCallPeerAddress.setAddress(peerIp);
                currentCallPeerPort = peerPort;

                callWin = new callWindow();
                callWin->setAttribute(Qt::WA_DeleteOnClose); // 窗口关闭时自动删除

                connect(callWin, &callWindow::accepted, this, &ChatWindow::onAcceptClicked);
                connect(callWin, &callWindow::rejected, this, &ChatWindow::onRejectClicked);
                connect(callWin, &QObject::destroyed, this, [this](){ callWin = nullptr; }); // <--- 安全措施
                callWin->showIncomingCall(peerName);
                // 弹出“来电”窗口让用户选择，而不是直接启动音频
                callWin->showIncomingCall(peerName);
            }else if(type == "hangup_call"){//处理“对方挂断”的通知
                qDebug() << "收到对方的挂断通知。";

                // 对方挂断了电话，我们也需要停止音频并更新UI
                stopAudio();


                //callWin->closeAndReset();
                // ui->hangupButton->setVisible(false);
                // ui->callButton->setVisible(true);
            }else if (type == "accept_call") {//对方接听了我们的电话
                if (callWin) {
                    // 此时我的 callWin 应该已经是 "通话中" 界面, 无需操作
                    qDebug() << currentCallPeerName << " 已接听，通话正式开始。";

                // 我们作为呼叫方，此时也应该启动音频
                //startAudio(QMediaDevices::defaultAudioInput(), QMediaDevices::defaultAudioOutput());

                // 并显示通话中窗口
                //callWin->showInCall(peerName);
                }
            }else if (type == "reject_call") {
                QString peerName = jsonObj["sender"].toString();
                qDebug() << peerName << " 拒绝了您的通话请求。";

                // 这里可以弹出一个提示，但为了简单，我们先只在控制台输出
                // 然后重置呼叫状态
                currentCallPeerName.clear();
                currentCallPeerAddress.clear();
                currentCallPeerPort = 0;

                stopAudio(); // 我方作为呼叫方，被拒绝后，停止呼叫过程
            }*/
            else if (type == "call_response" || type == "call_offer") {
                if (callWin) return; // 正在通话中，忽略新的请求

                QString peerName = jsonObj["peer_name"].toString();
                // ... 获取 ip/port ...
                currentCallPeerName = peerName;
                currentCallPeerAddress.setAddress(jsonObj["peer_ip"].toString());
                currentCallPeerPort = static_cast<quint16>(jsonObj["peer_port"].toInt());

                // 1. 创建窗口
                callWin = new callWindow();
                callWin->setAttribute(Qt::WA_DeleteOnClose);

                // 2. 连接信号
                // *** 这是最关键的连接：当窗口被销毁时，自动清理一切 ***
                connect(callWin, &QObject::destroyed, this, &ChatWindow::stopAudio);

                if (type == "call_response") { // 我是呼叫方
                    connect(callWin, &callWindow::hangedUp, this, &ChatWindow::onHangupClicked);
                    startAudio(QMediaDevices::defaultAudioInput(), QMediaDevices::defaultAudioOutput());
                    callWin->showInCall(peerName);
                } else { // 我是被叫方 ("call_offer")
                    connect(callWin, &callWindow::accepted, this, &ChatWindow::onAcceptClicked);
                    connect(callWin, &callWindow::rejected, this, &ChatWindow::onRejectClicked);
                    callWin->showIncomingCall(peerName);
                }
            } else if (type == "reject_call" || type == "hangup_call") {
                qDebug() << "收到对方的 " << type << " 通知，关闭通话窗口。";
                if (callWin) {
                    callWin->close(); // close() 将会触发 destroyed 信号，进而调用 stopAudio
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

    switchToOrOpenPrivateChat(clickedUser);

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


    // //模拟一下通话，测试
    // QJsonObject callRequest;
    // callRequest["type"] = "request_call";
    // callRequest["recipient"] = clickedUser; // 告诉服务器想和谁通话
    // sendMessage(callRequest); // 使用我们统一的发送函数
    // qDebug() << "向服务器发起与" << clickedUser << "的通话请求。";
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

void ChatWindow::switchToOrOpenPrivateChat(const QString &username)
{
    if (username == myUsername || username == "世界频道" || username.isEmpty()) {
        return;
    }

    if (sessionBrowsers.contains(username)) {
        // 如果存在，就遍历所有标签页，找到它并切换过去
        for (int i = 0; i < ui->chatTabWidget->count(); ++i) {
            if (ui->chatTabWidget->tabText(i) == username) {
                ui->chatTabWidget->setCurrentIndex(i);
                return; // 切换完成，函数结束
            }
        }
    } else{
        qDebug() << "为新用户" << username << "创建私聊窗口。";
        //创建UI组件
        QWidget *privateTab = new QWidget();
        QTextBrowser *privateBrowser = new QTextBrowser();
        QVBoxLayout *tabLayout = new QVBoxLayout(privateTab);
        tabLayout->setContentsMargins(0,0,0,0);
        tabLayout->addWidget(privateBrowser);

        // 将新标签页添加到TabWidget中
        int newIndex = ui->chatTabWidget->addTab(privateTab, username);

        sessionBrowsers.insert(username,privateBrowser);

        requestHistoryForChannel(username);

        // 自动切换到这个刚刚创建的新标签页
        ui->chatTabWidget->setCurrentIndex(newIndex);
    }

}

void ChatWindow::startAudio(const QAudioDevice &inputDevice, const QAudioDevice &outDevice)
{
    // 增加保护，如果音频设备已存在，则先停止并清理
    if (audioSource || audioSink) {
        qWarning() << "startAudio 被调用，但音频设备已存在。将先停止现有设备。";
        stopAudio();
    }

    // --- 1. 初始化音频输出 (扬声器) ---
    // 解释：创建一个 QAudioSink 对象，告诉它我们要用哪个设备（outputDevice）
    // 和哪种音频格式（audioFormat，我们之前在构造函数里定义的）。
    qInfo() << "正在初始化音频设备...";
    audioSink = new QAudioSink(outDevice,audioFormat,this);
    audioOutputDevice = audioSink->start();
    qInfo()<<"音频输出（扬声器）已启动。";
    // 解释：audioSink->start() 会返回一个 QIODevice 对象。
    // 这就像打开了一个文件，我们之后可以向这个“文件”写入数据，
    // 这些数据就会被送到扬声器播放出来。

    // --- 2. 初始化音频输入 (麦克风) ---
    //
    // 解释：和 AudioSink 类似，创建一个 QAudioSource 对象，
    // 指定要用的设备（inputDevice）和音频格式（audioFormat）。
    audioSource = new QAudioSource(inputDevice,audioFormat,this);
    audioInputDevice = audioSource->start();
    qInfo() << "音频输入（麦克风）已启动。";

    // --- 3. 连接信号与槽 ---
    //
    // 解释：这是实现音频数据流动的关键！
    // 我们告诉 audioInputDevice (代表麦克风的IO设备)，
    // 每当它有新的音频数据准备好时（即 readyRead() 信号被触发），
    // 就去调用我们的 onAudioInputReady() 槽函数。
    connect(audioInputDevice,&QIODevice::readyRead,this,&ChatWindow::onAudioInputReady);

}

void ChatWindow::stopAudio()
{
    // qInfo() << "正在停止音频设备...";

    // // 停止并销毁 QAudioSource (麦克风)
    // if(audioSource){
    //     // 在删除前先断开连接，这是更安全的做法
    //     if (audioInputDevice) {
    //         disconnect(audioInputDevice, &QIODevice::readyRead, this, &ChatWindow::onAudioInputReady);
    //     }
    //     audioSource->stop();
    //     delete audioSource;
    //     audioSource =  nullptr;
    // }

    // // 停止并销毁 QAudioSink (扬声器)
    // if(audioSink){
    //     audioSink->stop();
    //     delete audioSink;
    //     audioSink =  nullptr;
    // }
    // // audioInputDevice 和 audioOutputDevice 不需要手动 delete，
    // // 因为它们是 audioSource 和 audioSink 的一部分，会在上面被一并销毁。
    // audioInputDevice = nullptr;
    // audioOutputDevice = nullptr;

    // // 重置通话对端信息
    // currentCallPeerPort = 0;
    // currentCallPeerAddress.clear();
    // currentCallPeerName.clear();

    // // 3. 安全地关闭和销毁UI窗口
    // if (callWin) {
    //     // 断开所有与它的连接, 防止它在关闭过程中再次触发信号
    //     disconnect(callWin, nullptr, this, nullptr);
    //     callWin->close(); // 因为设置了WA_DeleteOnClose, close()就会触发销毁
    //     callWin = nullptr; // 立即将指针置空
    // }

    // qInfo() << "音频设备已停止。";
    qInfo() << "正在执行最终清理 (stopAudio)...";

    // 1. 停止并清理音频设备
    if (audioSource) {
        audioSource->stop();
        delete audioSource;
        audioSource = nullptr;
    }
    if (audioSink) {
        audioSink->stop();
        delete audioSink;
        audioSink = nullptr;
    }
    audioInputDevice = nullptr;
    audioOutputDevice = nullptr;

    // 2. 清理通话状态信息
    currentCallPeerPort = 0;
    currentCallPeerAddress.clear();
    currentCallPeerName.clear();

    // 3. 确保指针被置空
    // 此时 callWin 已经被销毁了，但为了绝对安全，我们检查并置空
    if (callWin) {
        callWin = nullptr;
    }
    qInfo() << "所有通话资源已清理。";
}

void ChatWindow::onUdpSocketReadyRead()
{
    if (!audioOutputDevice) {// 如果 audioOutputDevice 是 nullptr，说明通话还没开始或已经结束
        return;
    }

    // 只要socket里有数据，就一直循环读取
    while(udpSocket->hasPendingDatagrams()){
        // 创建一个足够大的缓冲区来接收数据
        QNetworkDatagram networkDatagram = udpSocket->receiveDatagram();// 它会自动创建一个合适大小的 QByteArray 来存放数据包，并返回它。
        const QByteArray &audioData = networkDatagram.data();
        // 直接用 write() 函数写入到 audioOutputDevice (扬声器设备) 中。
        // Qt 会自动处理剩下的所有事情，将声音通过扬声器播放出来。
        if (!audioData.isEmpty()) {
            audioOutputDevice->write(audioData);
        }

        // QHostAddress senderAddress;
        // quint16 senderPort;

        // // 读取一个数据包，同时获取发送方的地址和端口
        // udpSocket->readDatagram(datagram.data(),datagram.size(),&senderAddress,&senderPort);

        // // 在控制台打印收到的消息
        // qDebug() << "收到来自" << senderAddress.toString() << ":" << senderPort << "的UDP消息:" << datagram;

        // //我们一个 "pong" 回复
        // if(datagram == "ping"){
        //     QByteArray pong = "pong";
        //     udpSocket->writeDatagram(pong,senderAddress,senderPort);
        //     qDebug() << "已向对方回复UDP 'pong'。";
        // }
    }
}

void ChatWindow::onAudioInputReady()
{
    // 检查一下通话对象是否存在，如果不存在（比如通话已挂断），就什么都不做。
    if (currentCallPeerPort == 0) {
        return;
    }

    // 解释：audioInputDevice->readAll() 从麦克风的缓冲区读取所有可用的新音频数据。
    // 返回的是一个 QByteArray，里面是原始的PCM音频数据。
    QByteArray audioData = audioInputDevice->readAll();

    // 解释：我们使用之前已经验证过的 udpSocket->writeDatagram() 函数，
    // 将刚刚从麦克风读取到的音频数据，直接发送到对方的UDP地址和端口。
    udpSocket->writeDatagram(audioData,currentCallPeerAddress,currentCallPeerPort);

}


void ChatWindow::on_callButton_clicked()
{
    int currentIndex = ui->chatTabWidget->currentIndex();
    if (currentIndex == -1) return; // 如果没有任何标签页，则不执行任何操作

    QString recipientName = ui->chatTabWidget->tabText(currentIndex);

    // 不能和世界频道通话，也不能和自己通话
    if (recipientName == "世界频道" || recipientName == myUsername) {
        qWarning() << "无效的通话对象:" << recipientName;
        return;
    }

    currentCallPeerName = recipientName; // 立刻记录呼叫对象

    QJsonObject callRequest;
    callRequest["type"] = "request_call";
    callRequest["recipient"] = recipientName;
    sendMessage(callRequest);

    qDebug() << "呼叫按钮被点击！";
}

void ChatWindow::onHangupClicked()
{
    qDebug() << "用户点击了挂断按钮。";
    if (!currentCallPeerName.isEmpty()) {
        QJsonObject hangupMsg;
        hangupMsg["type"] = "hangup_call";
        hangupMsg["recipient"] = currentCallPeerName;
        sendMessage(hangupMsg);
    }
    // 不再直接调用 stopAudio(), 而是关闭窗口，让信号链来处理
    if (callWin) {
        callWin->close();
    }
}

void ChatWindow::onAcceptClicked()
{
    qDebug() << "用户点击了接听按钮。";
    if (!callWin || currentCallPeerName.isEmpty()) return;

    // 1. 断开 "rejected" 信号，因为它不再需要
    disconnect(callWin, &callWindow::rejected, this, &ChatWindow::onRejectClicked);
    // 2. 连接 "hangedUp" 信号，因为现在可以挂断了
    connect(callWin, &callWindow::hangedUp, this, &ChatWindow::onHangupClicked);

    // ... (通知对方、启动音频、更新UI 的代码不变)
    QJsonObject acceptMsg;
    acceptMsg["type"] = "accept_call";
    acceptMsg["recipient"] = currentCallPeerName;
    sendMessage(acceptMsg);

    startAudio(QMediaDevices::defaultAudioInput(), QMediaDevices::defaultAudioOutput());

    callWin->showInCall(currentCallPeerName);
}

void ChatWindow::onRejectClicked()
{
    qDebug() << "用户点击了拒绝按钮。";
    if (!currentCallPeerName.isEmpty()) {
        QJsonObject rejectMsg;
        rejectMsg["type"] = "reject_call";
        rejectMsg["recipient"] = currentCallPeerName;
        sendMessage(rejectMsg);
    }
    if (callWin) {
        callWin->close();
    }
}
