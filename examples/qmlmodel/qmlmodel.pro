QT       += core sql qml quick quickcontrols2

TARGET = qmlmodel
CONFIG   += c++17 qmltypes
CONFIG   -= app_bundle

QML_IMPORT_NAME = Qivot
QML_IMPORT_MAJOR_VERSION = 1

TEMPLATE = app

# Q_GADGET / QML_ELEMENT headers must be in HEADERS so moc + qmltyperegistrar see them.
HEADERS += post.h poststore.h
SOURCES += main.cpp poststore.cpp
RESOURCES += qml.qrc

include(../../src/qivot.pri)
