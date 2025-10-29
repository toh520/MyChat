#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QCloseEvent>
#include <QMessageBox>

//用json来传输数据
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    //自己重写关闭函数，增加功能
    void closeEvent(QCloseEvent *event) override;

private slots:
    void on_loginButton_clicked();//登录
    void onSocketDisconnected();//点击窗口后，在客户端断开连接后实现程序关闭
    void onSocketConnected();//处理连接成功的事件（点击登录按钮时）
    void onSocketReadyRead(); // 用于处理服务器发来新数据的槽函数


private:
    Ui::MainWindow *ui;
    QTcpSocket *socket;
    bool canClose = false;//用来解决关闭的递归问题，避免死循环
    bool login_Close=false;//判断断开连接时关闭窗口还是登录失败

};
#endif // MAINWINDOW_H
