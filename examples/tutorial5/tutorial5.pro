QT       += core
QT       -= gui

TARGET = tutorial5
CONFIG   += console
CONFIG   += c++17
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += main.cpp

include(../../src/qivot.pri)
