QT       += core sql qml quick quickcontrols2

# The wasm module's JS entry point is derived from TARGET and must be a valid
# identifier, so keep it hyphen-free (the project/directory is qivot-wasm-demo).
TARGET = qivotwasmdemo
CONFIG   += c++17 qmltypes
CONFIG   -= app_bundle

QML_IMPORT_NAME = Qivot
QML_IMPORT_MAJOR_VERSION = 1

TEMPLATE = app

HEADERS += note.h notestore.h
SOURCES += main.cpp notestore.cpp
RESOURCES += qml.qrc

# WebAssembly is a static build, so the SQLite driver plugin must be linked in
# explicitly (native/dynamic builds load it at runtime and ignore this).
wasm: QTPLUGIN += qsqlite

include(../../src/qivot.pri)
