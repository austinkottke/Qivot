QT       += core sql qml quick quickcontrols2

TARGET = contacts
CONFIG   += c++17 qmltypes
CONFIG   -= app_bundle

QML_IMPORT_NAME = Qivot
QML_IMPORT_MAJOR_VERSION = 1

TEMPLATE = app

HEADERS += contact.h contactstore.h
SOURCES += main.cpp contactstore.cpp
RESOURCES += qml.qrc

include(../../src/qivot.pri)
