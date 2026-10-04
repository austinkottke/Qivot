#-------------------------------------------------
#
# Project created by QtCreator 2014-02-19T17:14:29
#
#-------------------------------------------------

QT       += core
QT       += testlib
QT       += concurrent
QT       -= gui
# Phone apps start through Qt's platform plugin, which needs gui even with no UI
# (iOS: the qt_main_wrapper entry point; Android: the qtforandroid launcher).
ios|android: QT += gui

TARGET = unittests
CONFIG   += console
!ios: CONFIG -= app_bundle   # iOS only runs apps packaged as bundles

TEMPLATE = app


SOURCES += main.cpp \
    testobjectrunner.cpp \
    sqlitetests.cpp \
    coretests.cpp \
    dialecttests.cpp \
    schematests.cpp \
    validationtests.cpp

HEADERS += \
    testobjectrunner.h \
    coretests.h \
    sqlitetests.h \
    dialecttests.h \
    schematests.h \
    validationtests.h

include (../../src/qivot.pri)
include(../models/models.pri)
