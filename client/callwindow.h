#ifndef CALLWINDOW_H
#define CALLWINDOW_H

#include <QWidget>

namespace Ui {
class callWindow;
}

class callWindow : public QWidget
{
    Q_OBJECT

public:
    explicit callWindow(QWidget *parent = nullptr);
    ~callWindow();

private:
    Ui::callWindow *ui;


public:
    // 公开方法，用于从外部控制窗口状态
    void showIncomingCall(const QString &callerName);
    void showInCall(const QString &peerName);

signals:
    // 定义信号，用于通知 ChatWindow 用户的操作
    void accepted();
    void rejected();
    void hangedUp();

private slots:
    // 内部槽函数，响应UI按钮点击
    void on_acceptButton_clicked();
    void on_rejectButton_clicked();
    void on_hangupButton_clicked();

};

#endif // CALLWINDOW_H
