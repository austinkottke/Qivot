# Regression test for chained filter() — see QiSharedQuery::filter.
#
# Standalone rather than a slot in tests/unittests, because that target does
# not currently link: unittests.pro omits qimigrator.cpp, so QiMigrator and the
# QiError symbols are unresolved regardless of this test.
QT += core sql
CONFIG += console c++14
CONFIG -= app_bundle
TARGET = filterchain
SOURCES += main.cpp
INCLUDEPATH += $$PWD/../../dist
