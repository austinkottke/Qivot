QT       += core sql qml quick quickcontrols2 concurrent

TARGET = dashboard
CONFIG   += c++17 qmltypes
CONFIG   -= app_bundle

QML_IMPORT_NAME = Qivot
QML_IMPORT_MAJOR_VERSION = 1

TEMPLATE = app

HEADERS += dashboardstore.h models.h
SOURCES += main.cpp dashboardstore.cpp
RESOURCES += qml.qrc

include(../../src/qivot.pri)
