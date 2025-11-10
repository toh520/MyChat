/********************************************************************************
** Form generated from reading UI file 'chatwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.7.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CHATWINDOW_H
#define UI_CHATWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ChatWindow
{
public:
    QListWidget *userListWidget;
    QPushButton *sendButton;
    QLineEdit *messageLineEdit;
    QTabWidget *chatTabWidget;
    QPushButton *callButton;
    QPushButton *recordButton;

    void setupUi(QWidget *ChatWindow)
    {
        if (ChatWindow->objectName().isEmpty())
            ChatWindow->setObjectName("ChatWindow");
        ChatWindow->resize(376, 263);
        userListWidget = new QListWidget(ChatWindow);
        userListWidget->setObjectName("userListWidget");
        userListWidget->setGeometry(QRect(10, 10, 121, 221));
        sendButton = new QPushButton(ChatWindow);
        sendButton->setObjectName("sendButton");
        sendButton->setGeometry(QRect(320, 210, 51, 18));
        messageLineEdit = new QLineEdit(ChatWindow);
        messageLineEdit->setObjectName("messageLineEdit");
        messageLineEdit->setGeometry(QRect(140, 210, 111, 19));
        chatTabWidget = new QTabWidget(ChatWindow);
        chatTabWidget->setObjectName("chatTabWidget");
        chatTabWidget->setGeometry(QRect(140, 10, 171, 181));
        callButton = new QPushButton(ChatWindow);
        callButton->setObjectName("callButton");
        callButton->setGeometry(QRect(320, 20, 56, 18));
        recordButton = new QPushButton(ChatWindow);
        recordButton->setObjectName("recordButton");
        recordButton->setGeometry(QRect(260, 210, 56, 18));

        retranslateUi(ChatWindow);

        chatTabWidget->setCurrentIndex(-1);


        QMetaObject::connectSlotsByName(ChatWindow);
    } // setupUi

    void retranslateUi(QWidget *ChatWindow)
    {
        ChatWindow->setWindowTitle(QCoreApplication::translate("ChatWindow", "Form", nullptr));
        sendButton->setText(QCoreApplication::translate("ChatWindow", "\345\217\221\351\200\201", nullptr));
        callButton->setText(QCoreApplication::translate("ChatWindow", "\350\257\255\351\237\263\351\200\232\350\257\235", nullptr));
        recordButton->setText(QCoreApplication::translate("ChatWindow", "\346\214\211\344\275\217\345\275\225\351\237\263", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ChatWindow: public Ui_ChatWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CHATWINDOW_H
