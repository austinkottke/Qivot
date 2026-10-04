QT       += core sql qml quick quickcontrols2

TARGET = forms
CONFIG   += c++17
CONFIG   -= app_bundle

TEMPLATE = app

HEADERS += models.h
SOURCES += main.cpp
RESOURCES += qml.qrc

# QiForm and the Qivot.Forms controls (pulls in qivot.pri).
include(../../src/qivot-qml.pri)
