/****************************************************************************
** Meta object code from reading C++ file 'customtitlebar.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../src/ui/customtitlebar.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'customtitlebar.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.1. It"
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
struct qt_meta_tag_ZN14CustomTitleBarE_t {};
} // unnamed namespace

template <> constexpr inline auto CustomTitleBar::qt_create_metaobjectdata<qt_meta_tag_ZN14CustomTitleBarE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "CustomTitleBar",
        "minimizeClicked",
        "",
        "maximizeRestoreClicked",
        "closeClicked",
        "translateModeClicked",
        "realTimeModeClicked",
        "imageTranslationClicked",
        "relationsModeClicked"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'minimizeClicked'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'maximizeRestoreClicked'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'closeClicked'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'translateModeClicked'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'realTimeModeClicked'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'imageTranslationClicked'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'relationsModeClicked'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<CustomTitleBar, qt_meta_tag_ZN14CustomTitleBarE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject CustomTitleBar::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14CustomTitleBarE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14CustomTitleBarE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14CustomTitleBarE_t>.metaTypes,
    nullptr
} };

void CustomTitleBar::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<CustomTitleBar *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->minimizeClicked(); break;
        case 1: _t->maximizeRestoreClicked(); break;
        case 2: _t->closeClicked(); break;
        case 3: _t->translateModeClicked(); break;
        case 4: _t->realTimeModeClicked(); break;
        case 5: _t->imageTranslationClicked(); break;
        case 6: _t->relationsModeClicked(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (CustomTitleBar::*)()>(_a, &CustomTitleBar::minimizeClicked, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (CustomTitleBar::*)()>(_a, &CustomTitleBar::maximizeRestoreClicked, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (CustomTitleBar::*)()>(_a, &CustomTitleBar::closeClicked, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (CustomTitleBar::*)()>(_a, &CustomTitleBar::translateModeClicked, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (CustomTitleBar::*)()>(_a, &CustomTitleBar::realTimeModeClicked, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (CustomTitleBar::*)()>(_a, &CustomTitleBar::imageTranslationClicked, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (CustomTitleBar::*)()>(_a, &CustomTitleBar::relationsModeClicked, 6))
            return;
    }
}

const QMetaObject *CustomTitleBar::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CustomTitleBar::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14CustomTitleBarE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int CustomTitleBar::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void CustomTitleBar::minimizeClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void CustomTitleBar::maximizeRestoreClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void CustomTitleBar::closeClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void CustomTitleBar::translateModeClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void CustomTitleBar::realTimeModeClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void CustomTitleBar::imageTranslationClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void CustomTitleBar::relationsModeClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}
QT_WARNING_POP
