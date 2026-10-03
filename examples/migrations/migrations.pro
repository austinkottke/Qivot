QT       += core
QT       -= gui

TARGET = migrations
CONFIG   += console
CONFIG   += c++17
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += main.cpp
RESOURCES += migrations.qrc

include(../../src/qivot.pri)
