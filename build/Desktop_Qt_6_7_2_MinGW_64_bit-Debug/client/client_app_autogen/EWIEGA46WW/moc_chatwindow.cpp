/****************************************************************************
** Meta object code from reading C++ file 'chatwindow.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.7.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../client/chatwindow.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'chatwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.7.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {

#ifdef QT_MOC_HAS_STRINGDATA
struct qt_meta_stringdata_CLASSChatWindowENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSChatWindowENDCLASS = QtMocHelpers::stringData(
    "ChatWindow",
    "on_sendButton_clicked",
    "",
    "onSocketReadyRead",
    "onSocketDisconnected",
    "on_userListWidget_itemDoubleClicked",
    "QListWidgetItem*",
    "item",
    "onUdpSocketReadyRead",
    "onAudioInputReady",
    "on_callButton_clicked",
    "onHangupClicked",
    "onAcceptClicked",
    "onRejectClicked",
    "on_recordButton_pressed",
    "on_recordButton_released",
    "captureAudioData",
    "onVoiceMessageClicked",
    "url",
    "on_imageButton_clicked"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSChatWindowENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  104,    2, 0x08,    1 /* Private */,
       3,    0,  105,    2, 0x08,    2 /* Private */,
       4,    0,  106,    2, 0x08,    3 /* Private */,
       5,    1,  107,    2, 0x08,    4 /* Private */,
       8,    0,  110,    2, 0x08,    6 /* Private */,
       9,    0,  111,    2, 0x08,    7 /* Private */,
      10,    0,  112,    2, 0x08,    8 /* Private */,
      11,    0,  113,    2, 0x08,    9 /* Private */,
      12,    0,  114,    2, 0x08,   10 /* Private */,
      13,    0,  115,    2, 0x08,   11 /* Private */,
      14,    0,  116,    2, 0x08,   12 /* Private */,
      15,    0,  117,    2, 0x08,   13 /* Private */,
      16,    0,  118,    2, 0x08,   14 /* Private */,
      17,    1,  119,    2, 0x08,   15 /* Private */,
      19,    0,  122,    2, 0x08,   17 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 6,    7,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QUrl,   18,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject ChatWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_CLASSChatWindowENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSChatWindowENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSChatWindowENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<ChatWindow, std::true_type>,
        // method 'on_sendButton_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onSocketReadyRead'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onSocketDisconnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_userListWidget_itemDoubleClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QListWidgetItem *, std::false_type>,
        // method 'onUdpSocketReadyRead'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onAudioInputReady'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_callButton_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onHangupClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onAcceptClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onRejectClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_recordButton_pressed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_recordButton_released'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'captureAudioData'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onVoiceMessageClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QUrl &, std::false_type>,
        // method 'on_imageButton_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void ChatWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ChatWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->on_sendButton_clicked(); break;
        case 1: _t->onSocketReadyRead(); break;
        case 2: _t->onSocketDisconnected(); break;
        case 3: _t->on_userListWidget_itemDoubleClicked((*reinterpret_cast< std::add_pointer_t<QListWidgetItem*>>(_a[1]))); break;
        case 4: _t->onUdpSocketReadyRead(); break;
        case 5: _t->onAudioInputReady(); break;
        case 6: _t->on_callButton_clicked(); break;
        case 7: _t->onHangupClicked(); break;
        case 8: _t->onAcceptClicked(); break;
        case 9: _t->onRejectClicked(); break;
        case 10: _t->on_recordButton_pressed(); break;
        case 11: _t->on_recordButton_released(); break;
        case 12: _t->captureAudioData(); break;
        case 13: _t->onVoiceMessageClicked((*reinterpret_cast< std::add_pointer_t<QUrl>>(_a[1]))); break;
        case 14: _t->on_imageButton_clicked(); break;
        default: ;
        }
    }
}

const QMetaObject *ChatWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ChatWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSChatWindowENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int ChatWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 15;
    }
    return _id;
}
QT_WARNING_POP
