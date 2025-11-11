#include <QApplication>
#include "mainwindow.h"
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //<<< 新增加载 QSS 的代码 >>>
    // QFile styleFile(":/style.qss"); // 从资源文件中读取
    // if(styleFile.open(QFile::ReadOnly)){
    //     QString styleSheet = QLatin1String(styleFile.readAll());
    //     a.setStyleSheet(styleSheet);
    //     styleFile.close();
    // }

    QFile styleFile(":/style.qss"); // 从资源文件中读取
    if(styleFile.open(QFile::ReadOnly)){
        // 使用QLatin1String来避免编码问题
        QString styleSheet = QLatin1String(styleFile.readAll());
        a.setStyleSheet(styleSheet);
        styleFile.close();
        qDebug() << "QSS样式已成功加载！";
    } else {
        qWarning() << "无法加载QSS样式文件！";
    }
    //<<< QSS 加载结束 >>>

    MainWindow w;
    w.show();
    return a.exec();
}
