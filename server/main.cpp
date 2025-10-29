#include <QCoreApplication>
#include <QDebug> // 引入QDebug用于在控制台打印信息
#include "chatserver.h"

int main(int argc, char *argv[])
{
    // 对于没有界面的程序，我们使用QCoreApplication
    QCoreApplication a(argc, argv);

    // 打印一条启动信息，qInfo()是QDebug的一种，会输出到应用程序输出面板
    qInfo() << "=================================";
    qInfo() << "Server is starting...";
    qInfo() << "This is a console application.";
    qInfo() << "=================================";


    ChatServer server;
    server.startServer(8888);

    // 运行应用程序的事件循环，这样程序才不会立即退出
    return a.exec();
}
