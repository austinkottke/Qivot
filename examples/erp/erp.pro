QT       += core sql qml quick quickcontrols2

TARGET = erp
CONFIG   += c++17
CONFIG   -= app_bundle

TEMPLATE = app

HEADERS += models.h erpstore.h theme.h
SOURCES += main.cpp erpstore.cpp theme.cpp
RESOURCES += qml.qrc

# WebAssembly is a static build, so the SQLite driver plugin must be linked in
# explicitly (native/dynamic builds load it at runtime and ignore this).
wasm: QTPLUGIN += qsqlite

include(../../src/qivot.pri)
