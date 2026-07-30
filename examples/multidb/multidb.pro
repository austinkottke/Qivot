QT       += core sql
QT       -= gui

TARGET   = multidb
CONFIG   += c++17 console
CONFIG   -= app_bundle

TEMPLATE = app

HEADERS += product.h
SOURCES += main.cpp

include(../../src/qivot.pri)
