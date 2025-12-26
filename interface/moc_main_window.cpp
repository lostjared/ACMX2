/****************************************************************************
** Meta object code from reading C++ file 'main_window.hpp'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.18)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "main_window.hpp"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'main_window.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.18. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ReadOnlyStringListModel_t {
    QByteArrayData data[1];
    char stringdata0[24];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ReadOnlyStringListModel_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ReadOnlyStringListModel_t qt_meta_stringdata_ReadOnlyStringListModel = {
    {
QT_MOC_LITERAL(0, 0, 23) // "ReadOnlyStringListModel"

    },
    "ReadOnlyStringListModel"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ReadOnlyStringListModel[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void ReadOnlyStringListModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

QT_INIT_METAOBJECT const QMetaObject ReadOnlyStringListModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QStringListModel::staticMetaObject>(),
    qt_meta_stringdata_ReadOnlyStringListModel.data,
    qt_meta_data_ReadOnlyStringListModel,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ReadOnlyStringListModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ReadOnlyStringListModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ReadOnlyStringListModel.stringdata0))
        return static_cast<void*>(this);
    return QStringListModel::qt_metacast(_clname);
}

int ReadOnlyStringListModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QStringListModel::qt_metacall(_c, _id, _a);
    return _id;
}
struct qt_meta_stringdata_MainWindow_t {
    QByteArrayData data[20];
    char stringdata0[202];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MainWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
QT_MOC_LITERAL(0, 0, 10), // "MainWindow"
QT_MOC_LITERAL(1, 11, 12), // "fileOpenProp"
QT_MOC_LITERAL(2, 24, 0), // ""
QT_MOC_LITERAL(3, 25, 8), // "fileExit"
QT_MOC_LITERAL(4, 34, 11), // "runSelected"
QT_MOC_LITERAL(5, 46, 6), // "runAll"
QT_MOC_LITERAL(6, 53, 14), // "cameraSettings"
QT_MOC_LITERAL(7, 68, 11), // "listClicked"
QT_MOC_LITERAL(8, 80, 11), // "QModelIndex"
QT_MOC_LITERAL(9, 92, 1), // "i"
QT_MOC_LITERAL(10, 94, 7), // "newList"
QT_MOC_LITERAL(11, 102, 9), // "newShader"
QT_MOC_LITERAL(12, 112, 6), // "menuUp"
QT_MOC_LITERAL(13, 119, 8), // "menuDown"
QT_MOC_LITERAL(14, 128, 10), // "menuRemove"
QT_MOC_LITERAL(15, 139, 17), // "menuAudioSettings"
QT_MOC_LITERAL(16, 157, 8), // "menuSort"
QT_MOC_LITERAL(17, 166, 11), // "menuShuffle"
QT_MOC_LITERAL(18, 178, 10), // "menuSearch"
QT_MOC_LITERAL(19, 189, 12) // "menuFindNext"

    },
    "MainWindow\0fileOpenProp\0\0fileExit\0"
    "runSelected\0runAll\0cameraSettings\0"
    "listClicked\0QModelIndex\0i\0newList\0"
    "newShader\0menuUp\0menuDown\0menuRemove\0"
    "menuAudioSettings\0menuSort\0menuShuffle\0"
    "menuSearch\0menuFindNext"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   94,    2, 0x0a /* Public */,
       3,    0,   95,    2, 0x0a /* Public */,
       4,    0,   96,    2, 0x0a /* Public */,
       5,    0,   97,    2, 0x0a /* Public */,
       6,    0,   98,    2, 0x0a /* Public */,
       7,    1,   99,    2, 0x0a /* Public */,
      10,    0,  102,    2, 0x0a /* Public */,
      11,    0,  103,    2, 0x0a /* Public */,
      12,    0,  104,    2, 0x0a /* Public */,
      13,    0,  105,    2, 0x0a /* Public */,
      14,    0,  106,    2, 0x0a /* Public */,
      15,    0,  107,    2, 0x0a /* Public */,
      16,    0,  108,    2, 0x0a /* Public */,
      17,    0,  109,    2, 0x0a /* Public */,
      18,    0,  110,    2, 0x0a /* Public */,
      19,    0,  111,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 8,    9,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->fileOpenProp(); break;
        case 1: _t->fileExit(); break;
        case 2: _t->runSelected(); break;
        case 3: _t->runAll(); break;
        case 4: _t->cameraSettings(); break;
        case 5: _t->listClicked((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 6: _t->newList(); break;
        case 7: _t->newShader(); break;
        case 8: _t->menuUp(); break;
        case 9: _t->menuDown(); break;
        case 10: _t->menuRemove(); break;
        case 11: _t->menuAudioSettings(); break;
        case 12: _t->menuSort(); break;
        case 13: _t->menuShuffle(); break;
        case 14: _t->menuSearch(); break;
        case 15: _t->menuFindNext(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_MainWindow.data,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 16;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
