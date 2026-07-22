QT       += core network
QT       -= gui

TARGET = jsonhttp
CONFIG   += console
CONFIG   += c++17
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += main.cpp

# Use the network-enabled add-on .pri (pulls in QtNetwork + QiJsonRequest)
include(../../src/qivot-network.pri)
