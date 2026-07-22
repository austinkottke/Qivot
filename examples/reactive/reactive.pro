QT       += core sql qml quick quickcontrols2

TARGET = reactive
CONFIG   += c++17 qmltypes
CONFIG   -= app_bundle

QML_IMPORT_NAME = Qivot
QML_IMPORT_MAJOR_VERSION = 1

TEMPLATE = app

HEADERS += task.h taskstore.h
SOURCES += main.cpp taskstore.cpp
RESOURCES += qml.qrc

include(../../src/qivot.pri)
