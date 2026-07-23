QT       += core sql qml quick quickcontrols2

TARGET = infinitescroll
CONFIG   += c++17 qmltypes
CONFIG   -= app_bundle

QML_IMPORT_NAME = Qivot
QML_IMPORT_MAJOR_VERSION = 1

TEMPLATE = app

HEADERS += item.h itemstore.h
SOURCES += main.cpp itemstore.cpp
RESOURCES += qml.qrc

include(../../src/qivot.pri)
