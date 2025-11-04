/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.7.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QWidget *widget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_4;
    QLineEdit *ipLineEdit;
    QLabel *label;
    QHBoxLayout *horizontalLayout;
    QLabel *label_2;
    QLineEdit *accountLineEdit;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_3;
    QLineEdit *passwordLineEdit;
    QPushButton *loginButton;
    QLabel *errorLabel;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(317, 301);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        widget = new QWidget(centralwidget);
        widget->setObjectName("widget");
        widget->setGeometry(QRect(6, 0, 261, 221));
        verticalLayout = new QVBoxLayout(widget);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        label_4 = new QLabel(widget);
        label_4->setObjectName("label_4");

        horizontalLayout_3->addWidget(label_4);

        ipLineEdit = new QLineEdit(widget);
        ipLineEdit->setObjectName("ipLineEdit");

        horizontalLayout_3->addWidget(ipLineEdit);


        verticalLayout->addLayout(horizontalLayout_3);

        label = new QLabel(widget);
        label->setObjectName("label");
        QFont font;
        font.setPointSize(16);
        label->setFont(font);
        label->setAlignment(Qt::AlignmentFlag::AlignCenter);

        verticalLayout->addWidget(label);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        label_2 = new QLabel(widget);
        label_2->setObjectName("label_2");

        horizontalLayout->addWidget(label_2);

        accountLineEdit = new QLineEdit(widget);
        accountLineEdit->setObjectName("accountLineEdit");

        horizontalLayout->addWidget(accountLineEdit);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        label_3 = new QLabel(widget);
        label_3->setObjectName("label_3");

        horizontalLayout_2->addWidget(label_3);

        passwordLineEdit = new QLineEdit(widget);
        passwordLineEdit->setObjectName("passwordLineEdit");
        passwordLineEdit->setEchoMode(QLineEdit::EchoMode::Password);

        horizontalLayout_2->addWidget(passwordLineEdit);


        verticalLayout->addLayout(horizontalLayout_2);

        loginButton = new QPushButton(widget);
        loginButton->setObjectName("loginButton");

        verticalLayout->addWidget(loginButton);

        errorLabel = new QLabel(widget);
        errorLabel->setObjectName("errorLabel");
        errorLabel->setStyleSheet(QString::fromUtf8("color:red;"));

        verticalLayout->addWidget(errorLabel);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 317, 18));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MyChat Client", nullptr));
        label_4->setText(QCoreApplication::translate("MainWindow", "\346\234\215\345\212\241\345\231\250IP\357\274\232", nullptr));
        ipLineEdit->setText(QCoreApplication::translate("MainWindow", "127.0.0.1", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "Mychat\347\231\273\345\275\225", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "\350\264\246\345\217\267\357\274\232", nullptr));
        label_3->setText(QCoreApplication::translate("MainWindow", "\345\257\206\347\240\201\357\274\232", nullptr));
        loginButton->setText(QCoreApplication::translate("MainWindow", "\347\231\273\345\275\225", nullptr));
        errorLabel->setText(QCoreApplication::translate("MainWindow", "error", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
