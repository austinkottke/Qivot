# Optional Qivot add-on: QiForm, validating forms in QML, and the Qivot.Forms controls.
#
# Include this file INSTEAD OF qivot.pri when the app's UI is QML. It pulls in
# QtQml and QtQuick (which the core library does not require).
#
#   include(path/to/qivot/src/qivot-qml.pri)
#
# then, before loading QML:  qiRegisterQml(&engine);

QT += qml quick quickcontrols2

include($$PWD/qivot.pri)

INCLUDEPATH += $$PWD/qml
HEADERS += $$PWD/qml/qiform.h
SOURCES += $$PWD/qml/qiform.cpp
RESOURCES += $$PWD/qml/qivotqml.qrc
