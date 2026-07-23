QT       += core sql qml quick quickcontrols2

TARGET = savepoints
CONFIG   += c++17 qmltypes
CONFIG   -= app_bundle

QML_IMPORT_NAME = Qivot
QML_IMPORT_MAJOR_VERSION = 1

TEMPLATE = app

HEADERS += sandboxstore.h models.h
SOURCES += main.cpp sandboxstore.cpp
RESOURCES += qml.qrc

include(../../src/qivot.pri)
