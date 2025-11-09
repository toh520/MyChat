/********************************************************************************
** Form generated from reading UI file 'callwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.7.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CALLWINDOW_H
#define UI_CALLWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_callWindow
{
public:
    QStackedWidget *stackedWidget;
    QWidget *page;
    QPushButton *acceptButton;
    QPushButton *rejectButton;
    QLabel *incomingCallLabel;
    QWidget *page_2;
    QLabel *inCallStatusLabel;
    QPushButton *hangupButton;

    void setupUi(QWidget *callWindow)
    {
        if (callWindow->objectName().isEmpty())
            callWindow->setObjectName("callWindow");
        callWindow->resize(300, 150);
        stackedWidget = new QStackedWidget(callWindow);
        stackedWidget->setObjectName("stackedWidget");
        stackedWidget->setGeometry(QRect(0, 0, 291, 141));
        page = new QWidget();
        page->setObjectName("page");
        acceptButton = new QPushButton(page);
        acceptButton->setObjectName("acceptButton");
        acceptButton->setGeometry(QRect(40, 110, 56, 18));
        rejectButton = new QPushButton(page);
        rejectButton->setObjectName("rejectButton");
        rejectButton->setGeometry(QRect(140, 110, 56, 18));
        incomingCallLabel = new QLabel(page);
        incomingCallLabel->setObjectName("incomingCallLabel");
        incomingCallLabel->setGeometry(QRect(20, 10, 211, 21));
        stackedWidget->addWidget(page);
        page_2 = new QWidget();
        page_2->setObjectName("page_2");
        inCallStatusLabel = new QLabel(page_2);
        inCallStatusLabel->setObjectName("inCallStatusLabel");
        inCallStatusLabel->setGeometry(QRect(50, 50, 141, 16));
        hangupButton = new QPushButton(page_2);
        hangupButton->setObjectName("hangupButton");
        hangupButton->setGeometry(QRect(60, 110, 56, 18));
        stackedWidget->addWidget(page_2);

        retranslateUi(callWindow);

        stackedWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(callWindow);
    } // setupUi

    void retranslateUi(QWidget *callWindow)
    {
        callWindow->setWindowTitle(QCoreApplication::translate("callWindow", "\350\257\255\351\237\263\351\200\232\350\257\235", nullptr));
        acceptButton->setText(QCoreApplication::translate("callWindow", "\346\216\245\345\220\254", nullptr));
        rejectButton->setText(QCoreApplication::translate("callWindow", "\346\213\222\347\273\235", nullptr));
        incomingCallLabel->setText(QCoreApplication::translate("callWindow", "\346\255\243\345\234\250\345\244\204\347\220\206\346\235\245\347\224\265", nullptr));
        inCallStatusLabel->setText(QCoreApplication::translate("callWindow", "\346\255\243\345\234\250\351\200\232\350\257\235\344\270\255", nullptr));
        hangupButton->setText(QCoreApplication::translate("callWindow", "\346\214\202\346\226\255", nullptr));
    } // retranslateUi

};

namespace Ui {
    class callWindow: public Ui_callWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CALLWINDOW_H
