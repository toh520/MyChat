#include "chatwindow.h"
#include "callwindow.h"

#include "ui_chatwindow.h"
#include <QDebug>
#include <QJsonArray>
//#include <QDataStream>

#include <QTextBrowser>
#include <QVBoxLayout>

#include <QThread>

#include <QScrollBar>

#include <QFileDialog>
#include <QFileInfo>

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
    for(const QJsonValue &userValue : initialUsers){ // <--- 修改点
        QString user = userValue.toString();
        if(user == myUsername){
            ui->userListWidget->addItem(user + " (我)");
        } else {
            ui->userListWidget->addItem(user);
        }
    }

    currentPrivateChatUser = ""; // 初始化为空，表示当前是群聊模式

    //将私聊与群聊页面分开，创建群聊
    QWidget *worldChannelTab = new QWidget();
    QTextBrowser *worldChannelBrowser = new QTextBrowser();
    worldChannelBrowser->setOpenLinks(false); // <<< 【解决清屏问题】
    //布局管理器
    QVBoxLayout *tabLayout = new QVBoxLayout(worldChannelTab);
    tabLayout->setContentsMargins(0,0,0,0);// 让布局紧贴房间边缘，不留白
    tabLayout->addWidget(worldChannelBrowser); // 把“布告栏”放进布局里

    //加入界面中
    ui->chatTabWidget->addTab(worldChannelTab,"世界频道");
    connect(worldChannelBrowser, &QTextBrowser::anchorClicked, this, &ChatWindow::onVoiceMessageClicked);

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
    //callWin = new callWindow();
    //callWin->hide(); // 默认是隐藏的

    //信号与槽
    connect(socket,&QTcpSocket::readyRead,this,&ChatWindow::onSocketReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &ChatWindow::onSocketDisconnected);// 我们也需要处理断开连接的情况，以防服务器中途关闭
    connect(udpSocket,&QUdpSocket::readyRead,this,&ChatWindow::onUdpSocketReadyRead);

    //录音
    isPlayingVoiceMessage=false;


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

    // //=======================
    // // *** 在构造函数中就创建音频对象，并且只创建这一次！ ***
    // // 将它们作为 ChatWindow 的子对象，Qt 会自动管理内存
    // audioSource = new QAudioSource(QMediaDevices::defaultAudioInput(), audioFormat, this);
    // audioSink = new QAudioSink(QMediaDevices::defaultAudioOutput(), audioFormat, this);
    // //====================
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
        //currentBrowser->append(QString("<font color='green'>[我]:</font> %1").arg(text));
        // // 新代码：使用气泡
        // QString bubbleHtml = createBubbleHtml(text, true); // true代表是自己的消息
        // currentBrowser->insertHtml(bubbleHtml);

        QString currentTime = QDateTime::currentDateTime().toString("hh:mm:ss");
        QString header = QString(
                             "<div align='left' style='color: gray; font-size: 9pt;'>" // 新增 align='left'
                             "  <span style='color: lightgreen; font-weight: bold;'>我</span> %1"
                             "</div>"
                             ).arg(currentTime);

        QString body = QString(
                           "<div align='left' style='font-size: 11pt; margin-left: 10px; margin-bottom: 10px;'>%1</div>" // 新增 align='left'
                           ).arg(text.toHtmlEscaped());

        currentBrowser->append(header + body);
        currentBrowser->verticalScrollBar()->setValue(currentBrowser->verticalScrollBar()->maximum());
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
                    //browser->append(QString("[世界][%1]: %2").arg(sender).arg(text));

                    QString currentTime = QDateTime::currentDateTime().toString("hh:mm:ss");
                    // QString header = QString(
                    //                      "<div align='left' style='color: gray; font-size: 9pt;'>" // 新增 align='left'
                    //                      "  <span style='color: lightblue; font-weight: bold;'>%1</span> (世界频道) %2"
                    //                      "</div>"
                    //                      ).arg(sender, currentTime);
                    QString header;
                    if (sender == myUsername) {
                        // 如果是自己发的消息
                        header = QString(
                                     "<div align='left' style='color: gray; font-size: 9pt;'>"
                                     "  <span style='color: lightgreen; font-weight: bold;'>我</span> (世界频道) %1"
                                     "</div>"
                                     ).arg(currentTime);
                    } else {
                        // 如果是别人发的消息
                        header = QString(
                                     "<div align='left' style='color: gray; font-size: 9pt;'>"
                                     "  <span style='color: lightblue; font-weight: bold;'>%1</span> (世界频道) %2"
                                     "</div>"
                                     ).arg(sender, currentTime);
                    }
                    QString body = QString(
                                       "<div align='left' style='font-size: 11pt; margin-left: 10px; margin-bottom: 10px;'>%1</div>" // 新增 align='left'
                                       ).arg(text.toHtmlEscaped());

                    browser->append(header + body);

                    browser->verticalScrollBar()->setValue(browser->verticalScrollBar()->maximum());//定位到最下面
                }
                //ui->messageBrowser->append(QString("[世界][%1]: %2").arg(sender).arg(text));
            } else if (type == "user_list_update") {
                QJsonArray usersArray = jsonObj["users"].toArray();
                ui->userListWidget->clear();
                // for (const QJsonValue &user : usersArray) {
                //     ui->userListWidget->addItem(user.toString());
                // }
                for (const QJsonValue &userValue : usersArray) { // <--- 修改点
                    QString user = userValue.toString();
                    if(user == myUsername){
                        ui->userListWidget->addItem(user + " (我)");
                    } else {
                        ui->userListWidget->addItem(user);
                    }
                }
            }else if(type=="new_private_message"){
                QString sender = jsonObj["sender"].toString();
                QString text = jsonObj["text"].toString();

                switchToOrOpenPrivateChat(sender);

                QTextBrowser *browser = sessionBrowsers.value(sender);
                if(browser){
                    //填入消息内容
                    //browser->append(QString("<font color='blue'>[私聊] 来自 %1:</font> %2").arg(sender).arg(text));
                    // // 新代码：生成气泡
                    // QString bubbleHtml = createBubbleHtml(text, false); // false代表别人的消息
                    // browser->insertHtml(bubbleHtml);

                    QString currentTime = QDateTime::currentDateTime().toString("hh:mm:ss");
                    QString header = QString(
                                         "<div align='left' style='color: gray; font-size: 9pt;'>" // 新增 align='left'
                                         "  <span style='color: #00BFFF; font-weight: bold;'>%1</span> %2"
                                         "</div>"
                                         ).arg(sender, currentTime);
                    QString body = QString(
                                       "<div align='left' style='font-size: 11pt; margin-left: 10px; margin-bottom: 10px;'>%1</div>" // 新增 align='left'
                                       ).arg(text.toHtmlEscaped());

                    browser->append(header + body);
                    browser->verticalScrollBar()->setValue(browser->verticalScrollBar()->maximum());
                }
            }else if(type=="history_response"){
                QString channel = jsonObj["channel"].toString();
                QJsonArray history = jsonObj["history"].toArray();

                QTextBrowser *browser=sessionBrowsers.value(channel);
                if(browser){

                    // 先保存当前窗口已有的内容（也就是那条没来得及显示的第一条消息）
                    QString currentContent = browser->toHtml();

                    qDebug() << "===== currentContent 开头 =====";
                    qDebug() << currentContent.left(200);
                    qDebug() << "===== currentContent 结尾 =====";
                    qDebug() << currentContent.right(100);

                    // 先清空，再加载历史记录
                    browser->clear();

                    // 将历史消息逐条插入到显示窗口的顶部
                    for(const QJsonValue &msgValue:history){
                        QJsonObject msgObj = msgValue.toObject();
                        QString sender = msgObj["sender"].toString();
                        QString text = msgObj["text"].toString();
                        QString timestamp = msgObj["timestamp"].toString(); // 服务器记录的时间戳

                        bool isMyMessage = (sender == myUsername);

                        QString prefix = QString("[%1]: ").arg(sender);
                        if(text.startsWith(prefix)){
                            text = text.mid(prefix.length());
                        }

                        QString header;
                        if (sender == myUsername) {
                            header = QString("<div align='left' style='color: gray; font-size: 9pt;'><span style='color: lightgreen; font-weight: bold;'>我</span> %1</div>").arg(timestamp); // 新增 align='left'
                        } else {
                            header = QString("<div align='left' style='color: gray; font-size: 9pt;'><span style='color: lightblue; font-weight: bold;'>%1</span> %2</div>").arg(sender, timestamp); // 新增 align='left'
                        }
                        QString body = QString("<div align='left' style='font-size: 11pt; margin-left: 10px; margin-bottom: 10px;'>%1</div>").arg(text.toHtmlEscaped()); // 新增 align='left'

                        browser->append(header + body);

                        // 为群聊历史消息加上发送者前缀
                        // if (channel == "world_channel" && !isMyHistoryMessage) {
                        //     text = QString("[%1]: %2").arg(sender, text);
                        // }

                        // // 生成并插入气泡
                        // QString bubbleHtml = createBubbleHtml(text, isMyHistoryMessage);
                        // browser->insertHtml(bubbleHtml);

                        // 为了区分历史消息，我们可以用不同的颜色或格式
                        // QString formattedMessage;
                        // if (sender == myUsername) { // 如果是自己发的消息
                        //     formattedMessage = QString("<font color='gray'>[%1]</font> <font color='green'>[我]:</font> <font color='gray'>%2</font>")
                        //                            .arg(timestamp.left(19).replace("T", " "))
                        //                            .arg(text);
                        // } else { // 别人发的消息
                        //     formattedMessage = QString("<font color='gray'>[%1] [%2]: %3</font>")
                        //                            .arg(timestamp.left(19).replace("T", " "))
                        //                            .arg(sender)
                        //                            .arg(text);
                        // }
                        // browser->insertHtml(formattedMessage+"<br>");

                    }

                    // 在历史记录加载完后，显示一条的分割线
                    if (!history.isEmpty()) {
                        browser->append("<hr><em><p align='center' style='color:gray;'>--- 以上是历史消息 ---</p></em>");
                    }

                    // 最后，把之前保存的内容重新追加回来！
                    browser->append(currentContent);

                    browser->verticalScrollBar()->setValue(browser->verticalScrollBar()->maximum());
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
                connect(callWin, &QObject::destroyed, this, [this]() {
                    qDebug() << "通话窗口被销毁，确保音频已停止";
                    this->stopAudio();
                });

                if (type == "call_response") { // 我是呼叫方
                    connect(callWin, &callWindow::hangedUp, this, &ChatWindow::onHangupClicked);
                    startAudio(QMediaDevices::defaultAudioInput(), QMediaDevices::defaultAudioOutput());
                    callWin->showInCall(peerName);
                } else { // 我是被叫方 ("call_offer")
                    connect(callWin, &callWindow::accepted, this, &ChatWindow::onAcceptClicked);
                    connect(callWin, &callWindow::rejected, this, &ChatWindow::onRejectClicked);
                    callWin->showIncomingCall(peerName);
                }
            }else if (type == "accept_call") {
                qDebug() << "对方已接听，通话正式开始。";
                // 在这里，A 就明确地知道了 B 已经接听了电话
            }else if (type == "reject_call" || type == "hangup_call") {
                qDebug() << "收到对方的 " << type << " 通知，关闭通话窗口。";
                if (callWin) {
                    callWin->close(); // close() 将会触发 destroyed 信号，进而调用 stopAudio
                }
            }else if (type == "new_voice_message"){
                QString sender = jsonObj["sender"].toString();
                int duration_ms = jsonObj["duration_ms"].toInt();
                QString base64_data = jsonObj["data"].toString();
                QString channel = jsonObj["channel"].toString();
                // 1. Base64 解码
                QByteArray voiceData = QByteArray::fromBase64(base64_data.toUtf8());
                // 2. 生成一个唯一的消息 ID
                QString messageId = "voice_" + QString::number(QDateTime::currentMSecsSinceEpoch());
                // 3. 存储解码后的音频数据
                receivedVoiceMessages.insert(messageId, voiceData);
                // 4. 在 UI 上显示可点击的链接
                QString browserKey;
                QString displayText;
                QString currentTime = QDateTime::currentDateTime().toString("hh:mm:ss");
                QString header;
                if (channel == "世界频道") {
                    browserKey = "world_channel"; // 使用内部key
                    //isplayText = QString("[世界] 来自 %1: ").arg(sender);
                    header = QString(
                                 "<div align='left' style='color: gray; font-size: 9pt;'>"
                                 "  <span style='color: lightblue; font-weight: bold;'>%1</span> (世界频道) %2"
                                 "</div>"
                                 ).arg(sender, currentTime);
                } else {
                    // 如果是私聊，channel 就是私聊对象的名称。
                    // 但对于接收方来说，这条消息应该显示在与`sender`的聊天窗口里。
                    browserKey = sender;
                    //displayText = QString("来自 %1: ").arg(sender);

                    if (!sessionBrowsers.contains(sender)) {
                        qDebug() << "收到来自" << sender << "的第一条语音，自动创建窗口。";
                        switchToOrOpenPrivateChat(sender);
                    }

                    header = QString(
                                 "<div align='left' style='color: gray; font-size: 9pt;'>"
                                 "  <span style='color: #00BFFF; font-weight: bold;'>%1</span> %2"
                                 "</div>"
                                 ).arg(sender, currentTime);
                }

                QTextBrowser *browser = sessionBrowsers.value(browserKey);
                if(browser){
                    QString timeStr = QString::number(duration_ms / 1000.0, 'f', 1);
                    // 使用 <a> 标签创建一个链接，href 属性就是我们的唯一 ID
                    //QString voiceHtml = QString("<a href=\"%1\" style=\"text-decoration:none; color:blue;\">[点击播放 %2s 语音]</a>").arg(messageId).arg(timeStr);
                    //browser->append(displayText + voiceHtml);
                    QString voiceLink = QString("<a href=\"%1\" style=\"color:#5599FF; text-decoration:none;\">[点击播放 %2s 语音] 🎤</a>").arg(messageId).arg(timeStr);
                    QString body = QString(
                                       "<div align='left' style='font-size: 11pt; margin-left: 10px; margin-bottom: 10px;'>%1</div>"
                                       ).arg(voiceLink);

                    browser->append(header + body);

                    browser->verticalScrollBar()->setValue(browser->verticalScrollBar()->maximum());
                }
            }else if (type == "image_message" || type == "new_image_message") { // 同时处理两种类型
                QString sender = jsonObj["sender"].toString();
                QString base64_data = jsonObj["data"].toString();

                // 确定图片应该显示在哪个聊天窗口
                QString browserKey;
                QString headerText;
                QString currentTime = QDateTime::currentDateTime().toString("hh:mm:ss");

                if (type == "new_image_message") { // 根据类型判断是世界频道
                    browserKey = "world_channel";
                    headerText = QString(
                                     "<div align='left' style='color: gray; font-size: 9pt;'>"
                                     "  <span style='color: lightblue; font-weight: bold;'>%1</span> (世界频道) %2"
                                     "</div>"
                                     ).arg(sender, currentTime);
                } else { // image_message 就是私聊
                    browserKey = sender; // 对于接收方，私聊窗口的 key 永远是发送方的名字

                    // 如果是来自一个新朋友的第一条图片消息，我们必须先为他创建聊天窗口
                    if (!sessionBrowsers.contains(browserKey)) {
                        qDebug() << "收到来自新朋友 " << browserKey << " 的第一张图片，自动创建窗口。";
                        switchToOrOpenPrivateChat(browserKey);
                    }

                    headerText = QString(
                                     "<div align='left' style='color: gray; font-size: 9pt;'>"
                                     "  <span style='color: #00BFFF; font-weight: bold;'>%1</span> %2"
                                     "</div>"
                                     ).arg(sender, currentTime);
                }

                QTextBrowser *browser = sessionBrowsers.value(browserKey);
                if (browser) {
                    // 使用 <img> 标签显示图片
                    QString imageHtml = QString("<img src='data:image/jpeg;base64,%1' width='200'/>")
                                            .arg(base64_data);
                    QString body = QString(
                                       "<div align='left' style='font-size: 11pt; margin-left: 10px; margin-bottom: 10px;'>%1</div>"
                                       ).arg(imageHtml);

                    browser->append(headerText + body);
                    browser->verticalScrollBar()->setValue(browser->verticalScrollBar()->maximum());
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

    if (clickedUser.endsWith(" (我)")) {
        clickedUser = clickedUser.left(clickedUser.length() - 4);
    }


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

        privateBrowser->setOpenLinks(false); // <<< 【解决清屏问题】

        QVBoxLayout *tabLayout = new QVBoxLayout(privateTab);
        tabLayout->setContentsMargins(0,0,0,0);
        tabLayout->addWidget(privateBrowser);

        // 将新标签页添加到TabWidget中
        int newIndex = ui->chatTabWidget->addTab(privateTab, username);

        connect(privateBrowser, &QTextBrowser::anchorClicked, this, &ChatWindow::onVoiceMessageClicked);

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

    // 检查设备是否可用
    if (inputDevice.isNull() || outDevice.isNull()) {
        qCritical() << "音频设备不可用！";
        return;
    }

    // 检查格式是否支持
    if (!inputDevice.isFormatSupported(audioFormat) || !outDevice.isFormatSupported(audioFormat)) {
        qCritical() << "音频格式不被设备支持！";
        return;
    }

    // --- 1. 初始化音频输出 (扬声器) ---
    // 解释：创建一个 QAudioSink 对象，告诉它我们要用哪个设备（outputDevice）
    // 和哪种音频格式（audioFormat，我们之前在构造函数里定义的）。
    qInfo() << "正在初始化音频设备...";
    audioSink = new QAudioSink(outDevice,audioFormat,this);
    if (!audioSink) {
        qCritical() << "创建 QAudioSink 失败！";
        return;
    }
    audioOutputDevice = audioSink->start();
    if (!audioOutputDevice) {
        qCritical() << "启动音频输出设备失败！";
        return;
    }
    qInfo()<<"音频输出（扬声器）已启动。";
    // 解释：audioSink->start() 会返回一个 QIODevice 对象。
    // 这就像打开了一个文件，我们之后可以向这个“文件”写入数据，
    // 这些数据就会被送到扬声器播放出来。

    // --- 2. 初始化音频输入 (麦克风) ---
    //
    // 解释：和 AudioSink 类似，创建一个 QAudioSource 对象，
    // 指定要用的设备（inputDevice）和音频格式（audioFormat）。
    audioSource = new QAudioSource(inputDevice,audioFormat,this);
    if (!audioSource) {
        qCritical() << "创建 QAudioSource 失败！";
        return;
    }
    audioInputDevice = audioSource->start();
    if (!audioInputDevice) {
        qCritical() << "启动音频输入设备失败！";
        return;
    }
    qInfo() << "音频输入（麦克风）已启动。";

    // --- 3. 连接信号与槽 ---
    //
    // 解释：这是实现音频数据流动的关键！
    // 我们告诉 audioInputDevice (代表麦克风的IO设备)，
    // 每当它有新的音频数据准备好时（即 readyRead() 信号被触发），
    // 就去调用我们的 onAudioInputReady() 槽函数。
    connect(audioInputDevice,&QIODevice::readyRead,this,&ChatWindow::onAudioInputReady);
    qInfo() << "音频设备初始化完成。";
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


    //===================================================
    qInfo() << "正在停止音频设备...";

    // // 1. 先断开所有连接
    // if (audioInputDevice) {
    //     disconnect(audioInputDevice, &QIODevice::readyRead, this, &ChatWindow::onAudioInputReady);
    //     audioInputDevice = nullptr;

    // }

    // // 2. 停止并删除音频输入设备
    // if (audioSource) {
    //     QAudio::State state = audioSource->state();
    //     qInfo() << "停止音频输入设备，当前状态:" << state;

    //     audioSource->stop();
    //     // 等待一小段时间确保设备完全停止
    //     QThread::msleep(50);

    //     delete audioSource;
    //     audioSource = nullptr;
    // }

    // // 3. 停止并删除音频输出设备
    // if (audioSink) {
    //     QAudio::State state = audioSink->state();
    //     qInfo() << "停止音频输出设备，当前状态:" << state;

    //     audioSink->stop();
    //     // 等待一小段时间确保设备完全停止
    //     QThread::msleep(50);

    //     delete audioSink;
    //     audioSink = nullptr;
    // }

    // audioOutputDevice = nullptr;

    // 1. 停止并清理音频输入（麦克风）
    if (audioSource) {
        audioSource->stop();
        // 等待一小段时间确保设备完全停止
        QThread::msleep(50);
        // 当 audioSource 被 delete 时，Qt会自动断开所有与之相关的信号槽连接。
        // audioInputDevice 指针也会随着 audioSource 的销毁而失效。
        delete audioSource;
        audioSource = nullptr;
        audioInputDevice = nullptr; // 必须将指针置空，防止后续误用
    }
    qInfo() << "音频输入已清理。";

    // 2. 停止并清理音频输出（扬声器）
    if (audioSink) {
        audioSink->stop();
        // 等待一小段时间确保设备完全停止
        QThread::msleep(50);
        delete audioSink;
        audioSink = nullptr;
        audioOutputDevice = nullptr; // 将指针置空
    }
    qInfo() << "音频输出已清理。";

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

QString ChatWindow::createBubbleHtml(const QString &text, bool isMyMessage)
{
    // 定义两种气泡的样式
    QString bubbleStyle;
    QString alignSide;

    if (isMyMessage) {
        // 自己消息的样式：蓝色背景
        alignSide = "right";
        bubbleStyle = "background-color: #0078D7; color: white; padding: 8px 12px; border-radius: 10px;";
    } else {
        // 别人消息的样式：灰色背景
        alignSide = "left";
        bubbleStyle = "background-color: #4A4A4A; color: white; padding: 8px 12px; border-radius: 10px;";
    }

    // 构建一个更简单、更可靠的HTML结构
    // 使用 <p align="..."> 来确保每条消息占一行并正确对齐
    // 使用 <span> 作为气泡，因为它是行内元素，宽度会自适应内容
    QString html = QString(
                       "<p align='%1' style='margin: 0px 0px 8px 0px;'>" // <p>标签确保了换行和消息间距
                       "    <span style='%2'>%3</span>"                   // <span>作为气泡
                       "</p>"
                       ).arg(alignSide).arg(bubbleStyle).arg(text.toHtmlEscaped());

    return html;
}

// //======================
//     // 2. 修改 startAudio 函数
//     void ChatWindow::startAudio(const QAudioDevice &inputDevice, const QAudioDevice &outDevice)
// {
//     qInfo() << "正在启动音频流...";

//     // 不要再 new QAudioSource/Sink！直接使用已有的成员变量。

//     // 启动音频输出 (扬声器)
//     audioOutputDevice = audioSink->start();

//     // 启动音频输入 (麦克风)
//     audioInputDevice = audioSource->start();

//     // 连接麦克风的 readyRead 信号
//     connect(audioInputDevice, &QIODevice::readyRead, this, &ChatWindow::onAudioInputReady);

//     qInfo() << "音频流已启动。";
// }

// // 3. 修改 stopAudio 函数
// void ChatWindow::stopAudio()
// {
//     qInfo() << "正在执行最终清理 (stopAudio)...";

//     // 1. 停止音频流，但不要 delete 对象
//     if (audioSource && audioSource->state() != QAudio::StoppedState) {
//         // 必须在这里断开连接！
//         disconnect(audioInputDevice, &QIODevice::readyRead, this, &ChatWindow::onAudioInputReady);
//         audioSource->stop();
//     }
//     if (audioSink && audioSink->state() != QAudio::StoppedState) {
//         audioSink->stop();
//     }

//     // 将IO设备指针置空
//     audioInputDevice = nullptr;
//     audioOutputDevice = nullptr;
//     qInfo() << "音频流已停止。";

//     // 2. 清理通话状态信息 (这部分逻辑保持不变)
//     currentCallPeerPort = 0;
//     currentCallPeerAddress.clear();
//     currentCallPeerName.clear();

//     // 3. 确保UI指针被置空 (这部分逻辑保持不变)
//     if (callWin) {
//         callWin = nullptr;
//     }
//     qInfo() << "所有通话状态已清理。";
// }

// //===============================

void ChatWindow::onUdpSocketReadyRead()
{
    // if (!audioOutputDevice) {// 如果 audioOutputDevice 是 nullptr，说明通话还没开始或已经结束
    //     qDebug() << "audioOutputDevice 为 nullptr，无法播放音频";
    //     return;
    // }

    // 如果 audioOutputDevice 是 nullptr，说明通话还没开始或已经结束
    if (!audioOutputDevice) {
        qDebug() << "audioOutputDevice 为 nullptr，无法播放音频";
        // 在快速连续通话时，可能会收到上一次通话残留的UDP包，此时应该忽略
        // 为了避免这种情况，我们读取并丢弃所有待处理的数据包
        while(udpSocket->hasPendingDatagrams()){
            udpSocket->receiveDatagram();
        }
        return;
    }

    // 只要socket里有数据，就一直循环读取
    while(udpSocket->hasPendingDatagrams()){
        // 创建一个足够大的缓冲区来接收数据
        QNetworkDatagram networkDatagram = udpSocket->receiveDatagram();// 它会自动创建一个合适大小的 QByteArray 来存放数据包，并返回它。
        const QByteArray &audioData = networkDatagram.data();
        // 直接用 write() 函数写入到 audioOutputDevice (扬声器设备) 中。
        // Qt 会自动处理剩下的所有事情，将声音通过扬声器播放出来。
        qDebug() << "收到音频数据，大小:" << audioData.size()
                 << "从:" << networkDatagram.senderAddress().toString()
                 << ":" << networkDatagram.senderPort();

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
    qDebug() << "发送音频数据，大小:" << audioData.size()
             << "到:" << currentCallPeerAddress.toString() << ":" << currentCallPeerPort;

    if (!audioData.isEmpty()) {
        udpSocket->writeDatagram(audioData, currentCallPeerAddress, currentCallPeerPort);
    }
    // 解释：我们使用之前已经验证过的 udpSocket->writeDatagram() 函数，
    // 将刚刚从麦克风读取到的音频数据，直接发送到对方的UDP地址和端口。
    //udpSocket->writeDatagram(audioData,currentCallPeerAddress,currentCallPeerPort);

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

void ChatWindow::on_recordButton_pressed()
{
    qDebug() << "开始录制语音消息...";
    ui->recordButton->setText("松开发送");

    const QAudioDevice &inputDevice = QMediaDevices::defaultAudioInput();
    if (inputDevice.isNull()) {
        qWarning() << "没有找到可用的录音设备!";
        return;
    }

    //创建一个新的 QAudioSource 实例，专门用于这次录音
    voiceAudioSource = new QAudioSource(inputDevice,audioFormat,this);

    //    打开 QBuffer，准备接收数据
    //    QIODevice::WriteOnly | QIODevice::Truncate 表示以只写方式打开，并清空之前的所有内容

    voiceDataBuffer.open(QBuffer::WriteOnly | QBuffer::Truncate);

    voiceInputDevice = voiceAudioSource->start();

    if (!voiceInputDevice) {
        qWarning() << "启动录音设备失败!";
        // 清理资源
        voiceDataBuffer.close();
        delete voiceAudioSource;
        voiceAudioSource = nullptr;
        return;
    }

    //将 QAudioSource 的 readyRead 信号连接到我们的数据捕获槽函数
    connect(voiceInputDevice,&QIODevice::readyRead,this,&ChatWindow::captureAudioData);

    qDebug() << "录音设备已启动。";
}


void ChatWindow::on_recordButton_released()
{
    qDebug() << "录制结束。";
    ui->recordButton->setText("按住录音");

    // 如果 voiceAudioSource 存在，则停止录音
    if (voiceAudioSource) {
        voiceAudioSource->stop();
    }

    // 断开 voiceInputDevice 的信号连接
    if (voiceInputDevice) {
        disconnect(voiceInputDevice, &QIODevice::readyRead, this, &ChatWindow::captureAudioData);
    }

    // 安全地删除 QAudioSource 对象，并将指针置空
    // voiceInputDevice 是 voiceAudioSource 的一部分，不需要我们手动删除，它会随着父对象被销毁
    if (voiceAudioSource) {
        voiceAudioSource->deleteLater();
        voiceAudioSource = nullptr;
        voiceInputDevice = nullptr; // 将设备指针也置空
    }

    //  关闭缓冲区
    voiceDataBuffer.close();

    //检查我们是否录到了东西
    const QByteArray& recordedData = voiceDataBuffer.data();
    if (recordedData.isEmpty()) {
        qWarning() << "录音数据为空，不发送。";
        return;
    }

    qDebug() << "总共录制了 " << recordedData.size() << " 字节。准备发送...";

    // 计算录音时长 (毫秒)
    // 公式: 时长 = (总字节数 * 1000) / (采样率 * 声道数 * (采样位深 / 8)==2)
    int duration_ms = (recordedData.size()*1000)/ (audioFormat.sampleRate() * audioFormat.channelCount() * 2);

    qDebug() << "总共录制了 " << recordedData.size() << " 字节，时长约 " << duration_ms << " 毫秒。准备发送...";

    int currentIndex = ui->chatTabWidget->currentIndex();
    if (currentIndex == -1) {
        qWarning() << "没有选择聊天窗口，无法发送语音。";
        return;
    }
    QString recipient = ui->chatTabWidget->tabText(currentIndex);


    // 构建 JSON 对象
    QJsonObject voiceMessageObject;
    voiceMessageObject["type"] = "voice_message";
    voiceMessageObject["recipient"] = recipient;
    voiceMessageObject["duration_ms"] = duration_ms;
    voiceMessageObject["format"] = "s16le"; // 我们硬编码这个格式
    voiceMessageObject["data"] = QString(recordedData.toBase64()); // 转为 Base64 字符串

    sendMessage(voiceMessageObject);

    // 1. 生成一个唯一的消息ID (和接收方的逻辑一样)
    QString messageId = "voice_" + QString::number(QDateTime::currentMSecsSinceEpoch());

    // 2. 将录制的原始音频数据存到本地的 map 中
    receivedVoiceMessages.insert(messageId, recordedData);


    QString browserKey = recipient; // 默认key就是recipient的名字
    if (recipient == "世界频道") {
        browserKey = "world_channel"; // 如果是世界频道，则使用内部key
    }

    // 5. 在自己的聊天窗口显示一个提示
    QTextBrowser *currentBrowser = sessionBrowsers.value(browserKey);
    if(currentBrowser){
        QString timeStr = QString::number(duration_ms / 1000.0, 'f', 1); // 格式化为秒，保留一位小数

        QString currentTime = QDateTime::currentDateTime().toString("hh:mm:ss");
        QString header = QString(
                             "<div align='left' style='color: gray; font-size: 9pt;'>"
                             "  <span style='color: lightgreen; font-weight: bold;'>我</span> %1"
                             "</div>"
                             ).arg(currentTime);

        // 语音链接，使用一个更亮的颜色
        QString voiceLink = QString("<a href=\"%1\" style=\"color:#5599FF; text-decoration:none;\">[点击播放 %2s 语音] 🎤</a>").arg(messageId).arg(timeStr);
        QString body = QString(
                           "<div align='left' style='font-size: 11pt; margin-left: 10px; margin-bottom: 10px;'>%1</div>"
                           ).arg(voiceLink);

        currentBrowser->append(header + body);
        // 创建和接收方一样的HTML链接
        //QString voiceHtml = QString("<a href=\"%1\" style=\"text-decoration:none; color:green;\">[点击播放 %2s 语音]</a>").arg(messageId).arg(timeStr);

        // 追加到自己的窗口，注意我们把字体颜色改成了 'green' 来区分
        //currentBrowser->append(QString("<font color='green'>[我]:</font> %1").arg(voiceHtml));

        //currentBrowser->append(QString("<font color='green'>[我]:</font> [发送了一条 %1s 的语音]").arg(timeStr));
        currentBrowser->verticalScrollBar()->setValue(currentBrowser->verticalScrollBar()->maximum());
    }


}

void ChatWindow::captureAudioData()
{
    if (!voiceInputDevice) return;

    // 从 QAudioSource 读取所有可用的音频数据，并写入到我们的内存缓冲区中
    const QByteArray& data = voiceInputDevice->readAll();
    voiceDataBuffer.write(data);

    qDebug() << "捕获到 " << data.size() << " 字节的音频数据";

}

void ChatWindow::onVoiceMessageClicked(const QUrl &url)
{

    // 如果当前正在播放另一条语音，则忽略本次点击
    if (isPlayingVoiceMessage) {
        qDebug() << "正在播放其他语音，请稍后点击。";
        return;
    }

    QString messageId = url.toString();
    qDebug() << "语音消息被点击，ID:" << messageId;

    // 1. 检查我们是否存了这个ID的音频数据
    if (!receivedVoiceMessages.contains(messageId)) {
        qWarning() << "未找到 ID 为" << messageId << "的语音数据。";
        return;
    }

    // 2. 获取音频数据
    QByteArray voiceData = receivedVoiceMessages.value(messageId);

    // 让 buffer 拥有数据的所有权，而不是仅仅引用。
    QBuffer *buffer = new QBuffer();
    buffer->setData(voiceData);
    buffer->open(QIODevice::ReadOnly);

    isPlayingVoiceMessage = true; // 上锁

    // 1. 创建一个局部的、临时的 QAudioSink 对象。
    //    注意：这里没有指定父对象(this)，因为它将由自己管理生命周期。
    QAudioSink *tempAudioSink = new QAudioSink(QMediaDevices::defaultAudioOutput(), audioFormat);

    // 2. 连接 stateChanged 信号，这是实现自动销毁的关键！
    //    当播放状态从 Active/Suspended 变为 Idle (播放完毕) 或 Stopped 时，
    //    这个 lambda 表达式就会被调用。
    connect(tempAudioSink, &QAudioSink::stateChanged, this, [this,tempAudioSink](QAudio::State state) {
        if (state == QAudio::IdleState || state == QAudio::StoppedState) {
            qDebug() << "临时 AudioSink 播放完毕，状态：" << state << "，自动删除。";

            // 安全地删除自己。deleteLater() 会将删除操作排入事件队列，
            // 确保在槽函数执行完毕后才进行删除，非常安全。
            tempAudioSink->deleteLater();

            // 解锁
            this->isPlayingVoiceMessage = false;
        }
    });

    // QAudioSink::start() 返回一个 QIODevice，我们可以用它来直接播放一个现成的设备
    // 这里我们直接调用 start(buffer)，让它从头到尾播放 buffer 里的所有数据。
    // buffer 会在播放完成后被 QAudioSink 自动删除，非常方便。

    tempAudioSink->start(buffer);
    qDebug() << "正在播放语音消息(使用临时的 AudioSink 播放)";
}


void ChatWindow::on_imageButton_clicked()
{
    // 1. 打开文件选择对话框
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "选择一张图片",
        "", // 默认打开路径
        "图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)"
        );

    if (filePath.isEmpty()) {
        qDebug() << "没有选择任何文件。";
        return;
    }

    // 2. 读取文件内容
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开图片文件:" << filePath;
        return;
    }
    QByteArray imageData = file.readAll();
    file.close();

    //检查文件大小，避免发送过大的文件
    if (imageData.size() > 5 * 1024 * 1024) { // 限制为 5MB
        qWarning() << "图片文件过大 (超过5MB)，取消发送。";
        // 这里可以弹出一个提示框告诉用户
        return;
    }


    // 3. 获取当前聊天对象
    int currentIndex = ui->chatTabWidget->currentIndex();
    if (currentIndex == -1) {
        qWarning() << "没有选择聊天窗口，无法发送图片。";
        return;
    }
    QString recipient = ui->chatTabWidget->tabText(currentIndex);

    // 4. 构建 JSON 对象
    QJsonObject imageMessageObject;
    imageMessageObject["type"] = "image_message";
    imageMessageObject["recipient"] = recipient;
    imageMessageObject["filename"] = QFileInfo(filePath).fileName(); // 获取文件名
    imageMessageObject["data"] = QString(imageData.toBase64());    // 将图片数据转为 Base64

    // 5. 发送消息
    sendMessage(imageMessageObject);

    // 6. 在自己的聊天窗口立即显示图片
    QString browserKey = (recipient == "世界频道") ? "world_channel" : recipient;
    QTextBrowser *currentBrowser = sessionBrowsers.value(browserKey);
    if(currentBrowser){
        QString currentTime = QDateTime::currentDateTime().toString("hh:mm:ss");
        QString header = QString(
                             "<div align='left' style='color: gray; font-size: 9pt;'>"
                             "  <span style='color: lightgreen; font-weight: bold;'>我</span> %1"
                             "</div>"
                             ).arg(currentTime);

        // 使用 <img> 标签来显示 Base64 图片
        QString imageHtml = QString("<img src='data:image/jpeg;base64,%1' width='200'/>")
                                .arg(QString(imageData.toBase64()));

        QString body = QString(
                           "<div align='left' style='font-size: 11pt; margin-left: 10px; margin-bottom: 10px;'>%1</div>"
                           ).arg(imageHtml);

        currentBrowser->append(header + body);
        currentBrowser->verticalScrollBar()->setValue(currentBrowser->verticalScrollBar()->maximum());
    }
}

