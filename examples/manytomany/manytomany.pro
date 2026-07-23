QT       += core sql qml quick quickcontrols2 network

TARGET = manytomany
CONFIG   += c++17 qmltypes
CONFIG   -= app_bundle

QML_IMPORT_NAME = Qivot
QML_IMPORT_MAJOR_VERSION = 1

TEMPLATE = app

HEADERS += librarystore.h models.h
SOURCES += main.cpp librarystore.cpp
RESOURCES += qml.qrc

include(../../src/qivot.pri)
