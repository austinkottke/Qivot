# pri file for installation target

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
QT += sql

CONFIG += c++17

!msvc: QMAKE_CXXFLAGS += -Wno-invalid-offsetof

LIBS += -L$$PWD/../../lib/qivot -lqivot
