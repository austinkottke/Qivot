QT       += core sql qml quick quickcontrols2

TARGET = clinic
CONFIG   += c++17
CONFIG   -= app_bundle

TEMPLATE = app

HEADERS += models.h clinicstore.h theme.h
SOURCES += main.cpp clinicstore.cpp theme.cpp
RESOURCES += qml.qrc

# WebAssembly is a static build, so the SQLite driver plugin must be linked in
# explicitly (native/dynamic builds load it at runtime and ignore this). Qt's
# bundled SQLite ships with FTS5 compiled in, so note search works here too.
wasm: QTPLUGIN += qsqlite

include(../../src/qivot.pri)
