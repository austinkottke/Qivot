QT       += core sql qml quick quickcontrols2

TARGET = prefetch
CONFIG   += c++17 qmltypes
CONFIG   -= app_bundle

QML_IMPORT_NAME = Qivot
QML_IMPORT_MAJOR_VERSION = 1

TEMPLATE = app

HEADERS += prefetchstore.h models.h
SOURCES += main.cpp prefetchstore.cpp
RESOURCES += qml.qrc

include(../../src/qivot.pri)
