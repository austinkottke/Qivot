# pri file for installation target

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
QT += sql

CONFIG += c++17

QMAKE_CXXFLAGS += -Wno-invalid-offsetof

LIBS += -L$$PWD/../../lib/qivot -lqivot
