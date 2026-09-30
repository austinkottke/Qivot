QT       += core sql qml quick quickcontrols2

TARGET = contacts
CONFIG   += c++17 qmltypes
!ios: CONFIG -= app_bundle   # iOS only runs apps packaged as bundles

QML_IMPORT_NAME = Qivot
QML_IMPORT_MAJOR_VERSION = 1

TEMPLATE = app

HEADERS += contact.h contactstore.h
SOURCES += main.cpp contactstore.cpp
RESOURCES += qml.qrc

include(../../src/qivot.pri)

# Status bar / notch insets come from UIKit on iOS.
HEADERS += safearea.h
ios: OBJECTIVE_SOURCES += safearea_ios.mm
