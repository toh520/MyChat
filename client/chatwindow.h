#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QWidget>
#include <QTcpSocket>
#include <QMap>

//json
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>


class QListWidgetItem;
class QTextBrowser;

namespace Ui {
class ChatWindow;
}

class ChatWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWindow(QWidget *parent = nullptr);
    explicit ChatWindow(QTcpSocket *Socket,const QJsonArray &initialUsers,const QString &username, QWidget *parent = nullptr);//可以接受登录窗口来的socket
    ~ChatWindow();

private slots:
    void on_sendButton_clicked();//发送消息
    void onSocketReadyRead();// 处理收到的新消息
    void onSocketDisconnected();// 处理服务器断开的情况


    void on_userListWidget_itemDoubleClicked(QListWidgetItem *item);

private:
    Ui::ChatWindow *ui;
    QTcpSocket *socket;
    qint32 incompleteMessageSize = 0; // 用于处理“粘包/半包”问题
    QString currentPrivateChatUser;//当前私聊对象
    QString myUsername; // 存储当前登录的用户名
    QMap<QString, QTextBrowser*> sessionBrowsers;// 新增的核心数据结构：将会话ID映射到对应的消息显示框
};

#endif // CHATWINDOW_H
