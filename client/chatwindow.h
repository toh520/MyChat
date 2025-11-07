#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QWidget>
#include <QTcpSocket>
#include <QMap>
#include <QUdpSocket>
#include <QHostAddress> // 用于处理IP地址,QHostAddress 是 Qt 中专门用来表示 IP 地址的类，比用 QString 更专业、更安全。

//json
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>

#include <QDataStream>

//音频头文件
#include <QAudioSource>
#include <QAudioSink>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QNetworkDatagram>//接收音频数据

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
    void onUdpSocketReadyRead();//处理收到的UDP数据包的槽函数
    void onAudioInputReady();//当麦克风有新数据时调用的槽函数,从麦克风读取数据，然后通过 UDP 发送出去


    void on_callButton_clicked();

    void on_hangupButton_clicked();

private:
    Ui::ChatWindow *ui;
    QTcpSocket *socket;
    qint32 incompleteMessageSize = 0; // 用于处理“粘包/半包”问题
    QString currentPrivateChatUser;//当前私聊对象
    QString myUsername; // 存储当前登录的用户名
    QMap<QString, QTextBrowser*> sessionBrowsers;// 新增的核心数据结构：将会话ID映射到对应的消息显示框

    QUdpSocket *udpSocket;//用udp进行通话
    //存储当前通话对象的信息
    QHostAddress currentCallPeerAddress;
    quint16 currentCallPeerPort = 0;//是否为0来判断当前是否“在通话中”


private:
    void requestHistoryForChannel(const QString &channel);//用于请求指定频道的历史记录
    void sendMessage(const QJsonObject &message);//通用传输数据函数，解决socket》write的json粘包问题

private://音频变量
    QAudioSource *audioSource = nullptr;
    QAudioSink *audioSink = nullptr;
    QIODevice *audioInputDevice = nullptr;  // 用于从麦克风读取数据
    QIODevice *audioOutputDevice = nullptr; // 用于向扬声器写入数据
    QAudioFormat audioFormat;               // 用来存储我们定义的音频格式

private://音频函数
    void startAudio(const QAudioDevice &inputDevice,const QAudioDevice &outDevice);//负责初始化并启动 QAudioSource (麦克风) 和 QAudioSink (扬声器)。
    void stopAudio();//停止音频流的辅助函数
};

#endif // CHATWINDOW_H
