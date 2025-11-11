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
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ChatWindow
{
public:
    QHBoxLayout *horizontalLayout_2;
    QVBoxLayout *verticalLayout_2;
    QLabel *userListHeaderLabel;
    QListWidget *userListWidget;
    QWidget *widget;
    QVBoxLayout *verticalLayout;
    QTabWidget *chatTabWidget;
    QWidget *inputWidget;
    QHBoxLayout *horizontalLayout;
    QPushButton *recordButton;
    QPushButton *imageButton;
    QLineEdit *messageLineEdit;
    QPushButton *sendButton;
    QPushButton *callButton;

    void setupUi(QWidget *ChatWindow)
    {
        if (ChatWindow->objectName().isEmpty())
            ChatWindow->setObjectName("ChatWindow");
        ChatWindow->resize(800, 500);
        horizontalLayout_2 = new QHBoxLayout(ChatWindow);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName("verticalLayout_2");
        userListHeaderLabel = new QLabel(ChatWindow);
        userListHeaderLabel->setObjectName("userListHeaderLabel");

        verticalLayout_2->addWidget(userListHeaderLabel);

        userListWidget = new QListWidget(ChatWindow);
        userListWidget->setObjectName("userListWidget");

        verticalLayout_2->addWidget(userListWidget);


        horizontalLayout_2->addLayout(verticalLayout_2);

        widget = new QWidget(ChatWindow);
        widget->setObjectName("widget");
        verticalLayout = new QVBoxLayout(widget);
        verticalLayout->setObjectName("verticalLayout");
        chatTabWidget = new QTabWidget(widget);
        chatTabWidget->setObjectName("chatTabWidget");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(1);
        sizePolicy.setHeightForWidth(chatTabWidget->sizePolicy().hasHeightForWidth());
        chatTabWidget->setSizePolicy(sizePolicy);

        verticalLayout->addWidget(chatTabWidget);

        inputWidget = new QWidget(widget);
        inputWidget->setObjectName("inputWidget");
        horizontalLayout = new QHBoxLayout(inputWidget);
        horizontalLayout->setObjectName("horizontalLayout");
        recordButton = new QPushButton(inputWidget);
        recordButton->setObjectName("recordButton");

        horizontalLayout->addWidget(recordButton);

        imageButton = new QPushButton(inputWidget);
        imageButton->setObjectName("imageButton");

        horizontalLayout->addWidget(imageButton);

        messageLineEdit = new QLineEdit(inputWidget);
        messageLineEdit->setObjectName("messageLineEdit");

        horizontalLayout->addWidget(messageLineEdit);

        sendButton = new QPushButton(inputWidget);
        sendButton->setObjectName("sendButton");

        horizontalLayout->addWidget(sendButton);

        callButton = new QPushButton(inputWidget);
        callButton->setObjectName("callButton");

        horizontalLayout->addWidget(callButton);


        verticalLayout->addWidget(inputWidget);


        horizontalLayout_2->addWidget(widget);

        horizontalLayout_2->setStretch(0, 1);
        horizontalLayout_2->setStretch(1, 3);

        retranslateUi(ChatWindow);

        chatTabWidget->setCurrentIndex(-1);


        QMetaObject::connectSlotsByName(ChatWindow);
    } // setupUi

    void retranslateUi(QWidget *ChatWindow)
    {
        ChatWindow->setWindowTitle(QCoreApplication::translate("ChatWindow", "Form", nullptr));
        userListHeaderLabel->setText(QCoreApplication::translate("ChatWindow", "\345\234\250\347\272\277\347\224\250\346\210\267", nullptr));
        recordButton->setText(QCoreApplication::translate("ChatWindow", "\346\214\211\344\275\217\345\275\225\351\237\263", nullptr));
        imageButton->setText(QCoreApplication::translate("ChatWindow", "\345\233\276\347\211\207", nullptr));
        sendButton->setText(QCoreApplication::translate("ChatWindow", "\345\217\221\351\200\201", nullptr));
        callButton->setText(QCoreApplication::translate("ChatWindow", "\350\257\255\351\237\263\351\200\232\350\257\235", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ChatWindow: public Ui_ChatWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CHATWINDOW_H
