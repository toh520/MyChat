#include "mainwindow.h"
#include "ui_mainwindow.h" // 这个文件是Qt根据.ui文件自动生成的
#include <QDebug>
#include <QMessageBox>
#include <QCloseEvent>

//登录界面
#include "chatwindow.h"

#define IP "127.0.0.1"
#define PORT 8888

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //构造函数创建套接字socket套接字
    socket=new QTcpSocket(this);

    //连接信号与槽
    connect(socket,&QTcpSocket::disconnected,this,&MainWindow::onSocketDisconnected);
    connect(socket,&QTcpSocket::connected,this,&MainWindow::onSocketConnected);
    connect(socket,&QTcpSocket::readyRead,this,&MainWindow::onSocketReadyRead);


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(canClose){
        event->accept();
    }

    // 关键改动：检查socket的状态
    // 如果socket从未连接过，那么我们不需要走复杂的异步断开流程,可以直接点x关闭
    if(socket->state() == QAbstractSocket::UnconnectedState){
        qDebug() << "Socket从未连接，窗口直接关闭。";
        event->accept(); // 直接允许关闭
    }else{
        //关闭窗口
        login_Close=true;

        //额外增加关闭时主动断开socket
        qDebug()<<"窗口被关闭，正在主动断开与服务器的连接...";
        socket->disconnectFromHost();

        // //默认的关闭函数
        // MainWindow::closeEvent(event);
        //    告诉事件系统：“请忽略这次的关闭事件，不要关闭窗口！”
        //    我们稍后会手动关闭它。
        event->ignore();
    }

}

void MainWindow::on_loginButton_clicked()
{
    //获取相关信息
    QString account = ui->accountLineEdit->text();
    QString password = ui->passwordLineEdit->text();

    // qDebug()<<"登录按钮被点击";
    // qDebug()<<"输入的账号是:"<<account;
    // qDebug()<<"输入的密码是:"<<password;

    //  简单的前端校验
    if(account.isEmpty()||password.isEmpty()){
        //QMessageBox::warning(this,"登录失败","账号或者密码不能为空");
        ui->errorLabel->setText("账号或者密码不能为空");
        return;
    }

    //错误提示清空
    ui->errorLabel->setText("");

    qDebug()<<"正在尝试连接到服务器...";
    //获取ip地址
    QString serverIp = ui->ipLineEdit->text();
    // 8888是我们为服务器预留的端口号
    socket->connectToHost(serverIp,PORT);
    // 注意：connectToHost是异步操作，它会立即返回，
    // 连接是否成功，需要通过稍后的信号来判断。


}

void MainWindow::onSocketDisconnected()
{
    if(login_Close){
        qDebug() << "Socket已成功断开。现在，安全地关闭窗口。";

        canClose=true;//解决关闭闭环
        //手动关闭
        this->close();
    }else{
        qDebug() << "逻辑性断开（如登录失败）已完成，socket已重置";
    }

}

void MainWindow::onSocketConnected()
{
    qDebug()<<"与服务器连接成功！准备发送登录信息...";
    //获取相关信息
    QString account = ui->accountLineEdit->text();
    QString password = ui->passwordLineEdit->text();

    //用json对象存数据
    QJsonObject loginObject;
    loginObject["type"]="login";
    loginObject["account"]=account;
    loginObject["password"]=password;

    //将JSON对象转换为QByteArray
    QByteArray dataToSend=QJsonDocument(loginObject).toJson(QJsonDocument::Compact);

    //发送数据，socket->write(QByteArray)相当于Winsock的send()，异步，解决阻塞
    socket->write(dataToSend);
}

void MainWindow::onSocketReadyRead()
{
    QDataStream in(socket);
    in.setVersion(QDataStream::Qt_5_12);

    // 同样，我们只处理一次，因为登录响应后就交接给ChatWindow了
    if (incompleteMessageSize == 0) {
        if (socket->bytesAvailable() < (int)sizeof(qint32)) return;
        in >> incompleteMessageSize;
    }
    if (socket->bytesAvailable() < incompleteMessageSize) return;

    QByteArray messageData;
    messageData.resize(incompleteMessageSize);
    in.readRawData(messageData.data(), incompleteMessageSize);
    incompleteMessageSize = 0; // 重置

    // 解析登录响应
    QJsonDocument doc = QJsonDocument::fromJson(messageData);
    if(doc.isObject()){
        QJsonObject jsonObj = doc.object();
        if (jsonObj.contains("type") && jsonObj["type"].toString() == "login_response") {
            bool success = jsonObj["success"].toBool();
            QString message = jsonObj["message"].toString();
            if (success) {
                QJsonArray usersArray=jsonObj["users"].toArray();
                QString userName=ui->accountLineEdit->text();
                // 登录成功，交接给ChatWindow
                ChatWindow *chatWin = new ChatWindow(this->socket,usersArray,userName);
                chatWin->show();
                this->hide();
            } else {
                QMessageBox::warning(this, "登录失败", message);
                socket->disconnectFromHost();
            }
        }
    }
    // 注意：这里没有循环，因为我们预期登录响应后，数据流就由ChatWindow接管了

    // QByteArray data = socket->readAll();
    // qDebug()<<"收到服务器关于登录的回应："<<data;

    // //解析数据
    // QJsonParseError parseError;
    // QJsonDocument doc = QJsonDocument::fromJson(data,&parseError);

    // if(parseError.error!=QJsonParseError::NoError||!doc.isObject()){
    //     qWarning() << "解析服务器响应失败:" << parseError.errorString();
    //     return;
    // }
    // QJsonObject jsonObj=doc.object();

    // //json类型
    // if(jsonObj.contains("type")&&jsonObj["type"].toString()=="login_response"){
    //     bool success = jsonObj["success"].toBool();
    //     QString message = jsonObj["message"].toString();

    //     if(success){
    //         //QMessageBox::information(this,"登录成功",message);
    //         ChatWindow *chatWin = new ChatWindow(this->socket);//套接字传递过去
    //         chatWin->show();

    //         this->hide();

    //     }else{
    //         QMessageBox::warning(this,"登录失败",message);

    //         //断开连接
    //         socket->disconnectFromHost();
    //     }
    // }

}

