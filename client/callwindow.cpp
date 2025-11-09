#include "callwindow.h"
#include "ui_callwindow.h"

callWindow::callWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::callWindow)
{
    ui->setupUi(this);

    // 设置窗口总在最前
    setWindowFlag(Qt::WindowStaysOnTopHint);
}

callWindow::~callWindow()
{
    delete ui;
}

void callWindow::showIncomingCall(const QString &callerName)
{
    ui->incomingCallLabel->setText(callerName + " 向您发起语音通话...");
    ui->stackedWidget->setCurrentIndex(0); // 切换到“来电”页面
    this->show();
}

void callWindow::showInCall(const QString &peerName)
{
    ui->inCallStatusLabel->setText("正在与 " + peerName + " 通话中...");
    ui->stackedWidget->setCurrentIndex(1); // 切换到“通话中”页面
    this->show();
}

void callWindow::closeAndReset()
{
    this->hide();
}

void callWindow::on_acceptButton_clicked()
{
    emit accepted(); // 发射“已接听”信号

    // ChatWindow 在收到 accept_call 后会调用 showInCall
    // 我们在这里先切换页面，显示一个通用信息
    ui->inCallStatusLabel->setText("正在连接...");
    ui->stackedWidget->setCurrentIndex(1);
}

void callWindow::on_rejectButton_clicked()
{
    emit rejected(); // 发射“已拒绝”信号
    closeAndReset(); // 点击拒绝后，窗口自己就可以关掉了
}

void callWindow::on_hangupButton_clicked()
{
    emit hangedUp(); // 发射“已挂断”信号
    closeAndReset(); // 点击挂断后，窗口自己也可以关掉了
}
