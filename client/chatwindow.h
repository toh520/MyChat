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

#include <QBuffer>//用于在内存中读写数据


class QListWidgetItem;
class QTextBrowser;
class callWindow;

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

    void onHangupClicked();
    void onAcceptClicked();
    void onRejectClicked();

    //录音功能

    void on_recordButton_pressed();// 按下录音按钮

    void on_recordButton_released();// 松开录音按钮

    void captureAudioData();// 从麦克风捕获数据

    void onVoiceMessageClicked(const QUrl &url); // 用于处理语音消息点击事件

private:
    Ui::ChatWindow *ui;
    QTcpSocket *socket;
    qint32 incompleteMessageSize = 0; // 用于处理“粘包/半包”问题
    QString currentPrivateChatUser;//当前私聊对象
    QString myUsername; // 存储当前登录的用户名
    QMap<QString, QTextBrowser*> sessionBrowsers;// 新增的核心数据结构：将会话ID映射到对应的消息显示框

    QJsonObject firstMessageOfNewSession;//用于暂存新会话收到的第一条消息，以防止历史记录重复显示

    QUdpSocket *udpSocket;//用udp进行通话
    //存储当前通话对象的信息
    QHostAddress currentCallPeerAddress;
    quint16 currentCallPeerPort = 0;//是否为0来判断当前是否“在通话中”

    callWindow *callWin = nullptr;
    QString currentCallPeerName; // 存储当前通话对方的用户名


private:
    void requestHistoryForChannel(const QString &channel);//用于请求指定频道的历史记录
    void sendMessage(const QJsonObject &message);//通用传输数据函数，解决socket》write的json粘包问题
    void switchToOrOpenPrivateChat(const QString &username);//私聊窗口切换

private://音频变量
    QAudioSource *audioSource = nullptr;
    QAudioSink *audioSink = nullptr;
    // //===================尝试版本二解决音频问题
    // QAudioSource *audioSource; // 不要初始化为 nullptr
    // QAudioSink *audioSink;   // 不要初始化为 nullptr
    // //=====================
    QIODevice *audioInputDevice = nullptr;  // 用于从麦克风读取数据
    QIODevice *audioOutputDevice = nullptr; // 用于向扬声器写入数据
    QAudioFormat audioFormat;               // 用来存储我们定义的音频格式

    //录音
    QAudioSource *voiceAudioSource = nullptr;//用于录制语音消息的独立 QAudioSource
    QIODevice *voiceInputDevice = nullptr;    // [新增] 用于从麦克风读取语音消息数据的设备
    QBuffer voiceDataBuffer;// 用于暂存录音数据的内存缓冲区
    // 新增一个 Map, 用于存储收到的语音数据
    // 键是唯一的消息ID (例如时间戳), 值是解码后的音频 QByteArray
    QMap<QString, QByteArray> receivedVoiceMessages;
    bool isPlayingVoiceMessage = false;//判断是否在播放


private://音频函数
    void startAudio(const QAudioDevice &inputDevice,const QAudioDevice &outDevice);//负责初始化并启动 QAudioSource (麦克风) 和 QAudioSink (扬声器)。
    void stopAudio();//停止音频流的辅助函数

private://美化
    QString createBubbleHtml(const QString &text, bool isMyMessage);//// 用于生成消息气泡HTML的辅助函数
};

#endif // CHATWINDOW_H
