#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QWidget>
#include <QTcpSocket>


//json
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>

namespace Ui {
class ChatWindow;
}

class ChatWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWindow(QWidget *parent = nullptr);
    explicit ChatWindow(QTcpSocket *Socket,const QJsonArray &initialUsers, QWidget *parent = nullptr);//可以接受登录窗口来的socket
    ~ChatWindow();

private slots:
    void on_sendButton_clicked();//发送消息
    void onSocketReadyRead();// 处理收到的新消息
    void onSocketDisconnected();// 处理服务器断开的情况


private:
    Ui::ChatWindow *ui;
    QTcpSocket *socket;
    qint32 incompleteMessageSize = 0; // 用于处理“粘包/半包”问题
};

#endif // CHATWINDOW_H
