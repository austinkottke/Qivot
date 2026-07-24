QT       += core sql qml quick quickcontrols2

TARGET = fluxo
CONFIG   += c++17 qmltypes
CONFIG   -= app_bundle

QML_IMPORT_NAME = Qivot
QML_IMPORT_MAJOR_VERSION = 1

TEMPLATE = app

HEADERS += sample.h fluxview.h
SOURCES += main.cpp fluxview.cpp
RESOURCES += qml.qrc

include(../../src/qivot.pri)
