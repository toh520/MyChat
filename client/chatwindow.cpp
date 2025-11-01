#include "chatwindow.h"
#include "ui_chatwindow.h"
#include <QDebug>
#include <QJsonArray>
#include <QDataStream>

ChatWindow::ChatWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ChatWindow)
{
    ui->setupUi(this);
}

ChatWindow::ChatWindow(QTcpSocket *Socket,const QJsonArray &initialUsers, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ChatWindow)
{
    ui->setupUi(this);
    this->socket = Socket;

    // 关键一步：将socket的所有权转移到这个新窗口
    // 这样，当ChatWindow关闭时，socket也会被安全地销毁
    // 同时也断开了与原先MainWindow的父子关系

    // 1. 断开这个socket与之前所有对象的所有连接！
    //    这确保了MainWindow的槽函数不会再被触发。
    this->socket->disconnect();
    // 2. 将socket的父对象设置为当前窗口，确保内存被正确管理。
    this->socket->setParent(this);

    //信号与槽
    connect(socket,&QTcpSocket::readyRead,this,&ChatWindow::onSocketReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &ChatWindow::onSocketDisconnected);// 我们也需要处理断开连接的情况，以防服务器中途关闭

    //获取用户列表
    ui->userListWidget->clear();
    for(const QJsonValue &user:initialUsers){
        ui->userListWidget->addItem(user.toString());
    }


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
    messageObject["type"]="chat_message";
    messageObject["text"]=text;

    //json转换为qbytearray用来网络传输
    QByteArray dataToSend = QJsonDocument(messageObject).toJson(QJsonDocument::Compact);//参数QJsonDocument::Compact是一个枚举值，指定转换后的 JSON 格式为紧凑模式（即去除多余的空格、换行，生成一行紧凑的字符串），适合网络传输（减少数据量）或存储。

    socket->write(dataToSend);//send
    qDebug()<<"发送聊天消息："<<dataToSend;

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
                ui->messageBrowser->append(QString("[%1]: %2").arg(sender).arg(text));
            } else if (type == "user_list_update") {
                QJsonArray usersArray = jsonObj["users"].toArray();
                ui->userListWidget->clear();
                for (const QJsonValue &user : usersArray) {
                    ui->userListWidget->addItem(user.toString());
                }
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

