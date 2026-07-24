QT       += core sql qml quick quickcontrols2

TARGET = lexica
CONFIG   += c++17 qmltypes
CONFIG   -= app_bundle

QML_IMPORT_NAME = Qivot
QML_IMPORT_MAJOR_VERSION = 1

TEMPLATE = app

HEADERS += doc.h searchstore.h
SOURCES += main.cpp searchstore.cpp
RESOURCES += qml.qrc

include(../../src/qivot.pri)
