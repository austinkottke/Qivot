INCLUDEPATH += src/$$PWD

QT += sql

CONFIG += c++17

QMAKE_CXXFLAGS += -Wno-invalid-offsetof

TEMPLATE = lib
TARGET = qivot
CONFIG += staticlib
VERSION = 0.1

include (src/qivot.pri)

# Optionally bundle the JSON-over-HTTP loader (Qt Network) into the static
# library:  qmake CONFIG+=qivot_network
qivot_network {
    QT += network
    QIVOT_HEADERS += $$PWD/src/qijsonrequest.h
    HEADERS        += $$PWD/src/qijsonrequest.h
    SOURCES        += $$PWD/src/qijsonrequest.cpp
}

isEmpty(PREFIX) {
    PREFIX = /usr
}

LIBDIR=$$PREFIX/lib/qivot
INCDIR=$$PREFIX/include/qivot

INSTALLS += target headers
target.path = $$LIBDIR

headers.files = $$QIVOT_HEADERS
headers.path = $$INCDIR
headers.extra = cp $$PWD/src/qivot-install.pri ${INSTALL_ROOT}$$PREFIX/include/qivot/qivot.pri

