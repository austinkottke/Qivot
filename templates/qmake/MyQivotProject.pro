QT += core sql
CONFIG += c++17 console

# Uncomment for Qt Quick:
# QT += gui qml quick
# CONFIG -= app_bundle

TARGET = myapp
TEMPLATE = app

# Include Qivot
QIVOT_PATH = /path/to/qivot
include($$QIVOT_PATH/src/qivot.pri)

# Uncomment to enable JSON-over-HTTP support (requires QtNetwork)
# include($$QIVOT_PATH/src/qivot-network.pri)

SOURCES += \
    src/main.cpp

HEADERS += \
    src/models.h

# For Qt Quick templates, add:
# SOURCES += src/store.cpp
# HEADERS += src/store.h
# RESOURCES += qml/qml.qrc
