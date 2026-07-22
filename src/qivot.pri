INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
QT += sql

CONFIG += c++17

QMAKE_CXXFLAGS += -Wno-invalid-offsetof

QIVOT_HEADERS += \
    $$PWD/qierror.h \
    $$PWD/qilog.h \
    $$PWD/qiclause.h \
    $$PWD/qimodelmetainfo.h \
    $$PWD/qimodel.h \
    $$PWD/qiconnection.h \
    $$PWD/qibasefield.h \
    $$PWD/qisqlstatement.h \
    $$PWD/qisqlitestatement.h \
    $$PWD/qiwhere.h \
    $$PWD/qisql.h \
    $$PWD/qifield.h \
    $$PWD/qiforeignkey.h \
    $$PWD/qisharedquery.h \
    $$PWD/qiquery.h \
    $$PWD/qiqueryrules.h \
    $$PWD/qiexpression.h \
    $$PWD/qilist.h \
    $$PWD/qiabstractmodel.h \
    $$PWD/qisharedlist.h \
    $$PWD/qiindex.h \
    $$PWD/qistream.h \
    $$PWD/qilistwriter.h \
    $$PWD/qijoin.h \
    $$PWD/qifieldref.h \
    $$PWD/qicolumns.h \
    $$PWD/qirelation.h \
    $$PWD/qiftsindex.h \
    $$PWD/qitransaction.h \
    $$PWD/qigadget.h \
    $$PWD/qilistmodel.h \
    $$PWD/qijsonmapper.h \
    $$PWD/qivot.h

QIVOT_PRIV_HEADERS = \
    $$PWD/qiwhere_p.h \
    $$PWD/qisharedquery_p.h \
    $$PWD/qimetainfoquery_p.h

HEADERS += $$QIVOT_HEADERS
HEADERS += $$QIVOT_PRIV_HEADERS

SOURCES += \
    $$PWD/qilog.cpp \
    $$PWD/qiclause.cpp \
    $$PWD/qimodelmetainfo.cpp \
    $$PWD/qimodel.cpp \
    $$PWD/qiconnection.cpp \
    $$PWD/qibasefield.cpp \
    $$PWD/qisqlstatement.cpp \
    $$PWD/qisqlitestatement.cpp \
    $$PWD/qiwhere.cpp \
    $$PWD/qisql.cpp \
    $$PWD/qifield.cpp \
    $$PWD/qisharedquery.cpp \
    $$PWD/qiqueryrules.cpp \
    $$PWD/qiexpression.cpp \
    $$PWD/qiabstractmodel.cpp \
    $$PWD/qisharedlist.cpp \
    $$PWD/qiindex.cpp \
    $$PWD/qistream.cpp \
    $$PWD/qilistwriter.cpp \
    $$PWD/qijsonmapper.cpp \
    $$PWD/qilistmodel.cpp
