QT       += core sql qml quick quickcontrols2

TARGET = clinic
CONFIG   += c++17
CONFIG   -= app_bundle

TEMPLATE = app

HEADERS += models.h clinicstore.h theme.h
SOURCES += main.cpp clinicstore.cpp theme.cpp
RESOURCES += qml.qrc

include(../../src/qivot.pri)
