QT       += core sql qml quick quickcontrols2

TARGET   = vision
CONFIG   += c++17
CONFIG   -= app_bundle

TEMPLATE = app

HEADERS += models.h seeddata.h visionstore.h theme.h
SOURCES += main.cpp visionstore.cpp theme.cpp
RESOURCES += qml.qrc

include(../../src/qivot.pri)
