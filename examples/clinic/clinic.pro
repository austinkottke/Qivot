QT       += core sql qml quick quickcontrols2

TARGET = clinic
CONFIG   += c++17 qmltypes
CONFIG   -= app_bundle

QML_IMPORT_NAME = Qivot
QML_IMPORT_MAJOR_VERSION = 1

TEMPLATE = app

HEADERS += models.h clinicstore.h
SOURCES += main.cpp clinicstore.cpp
RESOURCES += qml.qrc

include(../../src/qivot.pri)
