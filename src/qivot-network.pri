# Optional DQuest add-on: JSON-over-HTTP loading on a worker thread.
#
# Include this file INSTEAD OF qivot.pri when you want QiJsonRequest. It pulls
# in the QtNetwork module (which the core library does not require) and adds the
# HTTP request sources on top of the core DQuest sources.
#
#   include(path/to/qivot/src/qivot-network.pri)

QT += network

include($$PWD/qivot.pri)

QIVOT_HEADERS += $$PWD/qijsonrequest.h

HEADERS += $$PWD/qijsonrequest.h
SOURCES += $$PWD/qijsonrequest.cpp
