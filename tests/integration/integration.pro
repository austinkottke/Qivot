QT       += core sql
QT       -= gui

TARGET   = integration
CONFIG   += c++17 console
CONFIG   -= app_bundle

TEMPLATE = app

HEADERS += intmodel.h
SOURCES += main.cpp

include(../../src/qivot.pri)
