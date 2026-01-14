/****************************************************************************
** Meta object code from reading C++ file 'menubar.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../src/ui/menubar.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'menubar.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN7MenuBarE_t {};
} // unnamed namespace

template <> constexpr inline auto MenuBar::qt_create_metaobjectdata<qt_meta_tag_ZN7MenuBarE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MenuBar",
        "openMockData",
        "",
        "newProject",
        "openProject",
        "settings",
        "saveProject",
        "deployProject",
        "exit",
        "fontManager",
        "pluginManager",
        "featureManager",
        "editEngineScript",
        "toggleContext",
        "checked",
        "hideCompleted",
        "exportSmartFilterRules",
        "importSmartFilterRules"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'openMockData'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'newProject'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'openProject'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'settings'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'saveProject'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'deployProject'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'exit'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'fontManager'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'pluginManager'
        QtMocHelpers::SignalData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'featureManager'
        QtMocHelpers::SignalData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'editEngineScript'
        QtMocHelpers::SignalData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'toggleContext'
        QtMocHelpers::SignalData<void(bool)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 14 },
        }}),
        // Signal 'hideCompleted'
        QtMocHelpers::SignalData<void(bool)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 14 },
        }}),
        // Signal 'exportSmartFilterRules'
        QtMocHelpers::SignalData<void()>(16, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'importSmartFilterRules'
        QtMocHelpers::SignalData<void()>(17, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MenuBar, qt_meta_tag_ZN7MenuBarE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject MenuBar::staticMetaObject = { {
    QMetaObject::SuperData::link<QMenuBar::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7MenuBarE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7MenuBarE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN7MenuBarE_t>.metaTypes,
    nullptr
} };

void MenuBar::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MenuBar *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->openMockData(); break;
        case 1: _t->newProject(); break;
        case 2: _t->openProject(); break;
        case 3: _t->settings(); break;
        case 4: _t->saveProject(); break;
        case 5: _t->deployProject(); break;
        case 6: _t->exit(); break;
        case 7: _t->fontManager(); break;
        case 8: _t->pluginManager(); break;
        case 9: _t->featureManager(); break;
        case 10: _t->editEngineScript(); break;
        case 11: _t->toggleContext((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 12: _t->hideCompleted((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 13: _t->exportSmartFilterRules(); break;
        case 14: _t->importSmartFilterRules(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (MenuBar::*)()>(_a, &MenuBar::openMockData, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (MenuBar::*)()>(_a, &MenuBar::newProject, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (MenuBar::*)()>(_a, &MenuBar::openProject, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (MenuBar::*)()>(_a, &MenuBar::settings, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (MenuBar::*)()>(_a, &MenuBar::saveProject, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (MenuBar::*)()>(_a, &MenuBar::deployProject, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (MenuBar::*)()>(_a, &MenuBar::exit, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (MenuBar::*)()>(_a, &MenuBar::fontManager, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (MenuBar::*)()>(_a, &MenuBar::pluginManager, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (MenuBar::*)()>(_a, &MenuBar::featureManager, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (MenuBar::*)()>(_a, &MenuBar::editEngineScript, 10))
            return;
        if (QtMocHelpers::indexOfMethod<void (MenuBar::*)(bool )>(_a, &MenuBar::toggleContext, 11))
            return;
        if (QtMocHelpers::indexOfMethod<void (MenuBar::*)(bool )>(_a, &MenuBar::hideCompleted, 12))
            return;
        if (QtMocHelpers::indexOfMethod<void (MenuBar::*)()>(_a, &MenuBar::exportSmartFilterRules, 13))
            return;
        if (QtMocHelpers::indexOfMethod<void (MenuBar::*)()>(_a, &MenuBar::importSmartFilterRules, 14))
            return;
    }
}

const QMetaObject *MenuBar::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MenuBar::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7MenuBarE_t>.strings))
        return static_cast<void*>(this);
    return QMenuBar::qt_metacast(_clname);
}

int MenuBar::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMenuBar::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 15;
    }
    return _id;
}

// SIGNAL 0
void MenuBar::openMockData()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void MenuBar::newProject()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void MenuBar::openProject()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void MenuBar::settings()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void MenuBar::saveProject()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void MenuBar::deployProject()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void MenuBar::exit()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void MenuBar::fontManager()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void MenuBar::pluginManager()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void MenuBar::featureManager()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void MenuBar::editEngineScript()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void MenuBar::toggleContext(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 11, nullptr, _t1);
}

// SIGNAL 12
void MenuBar::hideCompleted(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 12, nullptr, _t1);
}

// SIGNAL 13
void MenuBar::exportSmartFilterRules()
{
    QMetaObject::activate(this, &staticMetaObject, 13, nullptr);
}

// SIGNAL 14
void MenuBar::importSmartFilterRules()
{
    QMetaObject::activate(this, &staticMetaObject, 14, nullptr);
}
QT_WARNING_POP
