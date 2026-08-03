QT       += core sql qml quick quickcontrols2

TARGET   = vision
CONFIG   += c++17
CONFIG   -= app_bundle

TEMPLATE = app

HEADERS += models.h seeddata.h visionstore.h theme.h
SOURCES += main.cpp visionstore.cpp theme.cpp
RESOURCES += qml.qrc

# WebAssembly is a static build, so the SQLite driver plugin (loaded dynamically
# on desktop) must be linked in explicitly. The app uses an in-memory :memory:
# database, so nothing else changes — it runs entirely in the browser.
wasm: QTPLUGIN += qsqlite

include(../../src/qivot.pri)
