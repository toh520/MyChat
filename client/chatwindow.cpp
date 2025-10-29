#include "chatwindow.h"
#include "ui_chatwindow.h"

ChatWindow::ChatWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ChatWindow)
{
    ui->setupUi(this);
}

ChatWindow::ChatWindow(QTcpSocket *Socket, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ChatWindow)
{
    ui->setupUi(this);
    this->socket = Socket;

    // 关键一步：将socket的所有权转移到这个新窗口
    // 这样，当ChatWindow关闭时，socket也会被安全地销毁
    // 同时也断开了与原先MainWindow的父子关系
    this->socket->setParent(this);
}

ChatWindow::~ChatWindow()
{
    delete ui;
}
