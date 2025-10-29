#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QWidget>
#include <QTcpSocket>

namespace Ui {
class ChatWindow;
}

class ChatWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWindow(QWidget *parent = nullptr);
    explicit ChatWindow(QTcpSocket *Socket, QWidget *parent = nullptr);//可以接受登录窗口来的socket
    ~ChatWindow();

private:
    Ui::ChatWindow *ui;
    QTcpSocket *socket;
};

#endif // CHATWINDOW_H
