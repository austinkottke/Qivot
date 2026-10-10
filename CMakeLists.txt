cmake_minimum_required(VERSION 3.16)

project(Qivot
    VERSION 1.0.1
    DESCRIPTION "A modern C++17 Qt ORM for SQLite with JSON-over-HTTP"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

option(QIVOT_WITH_NETWORK "Build the JSON-over-HTTP loader (requires Qt Network)" ON)
option(QIVOT_BUILD_EXAMPLES "Build the example programs" OFF)
option(QIVOT_WITH_QML "Build Qivot::qml: QiForm and the Qivot.Forms controls (requires Qt Qml and Quick)" OFF)

# --- Qt (works with both Qt 5.15+ and Qt 6) ---
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Sql)
find_package(Qt${QT_VERSION_MAJOR} 5.15 REQUIRED COMPONENTS Core Sql)

set(QIVOT_QT_COMPONENTS Core Sql)
if(QIVOT_WITH_NETWORK)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Network)
    list(APPEND QIVOT_QT_COMPONENTS Network)
endif()

# --- Sources ---
set(QIVOT_SOURCES
    src/qilog.cpp
    src/qiclause.cpp
    src/qimodelmetainfo.cpp
    src/qimodel.cpp
    src/qiconnection.cpp
    src/qibasefield.cpp
    src/qisqlstatement.cpp
    src/qisqlitestatement.cpp
    src/qimysqlstatement.cpp
    src/qipgstatement.cpp
    src/qimssqlstatement.cpp
    src/qioraclestatement.cpp
    src/qiduckdbstatement.cpp
    src/qiwhere.cpp
    src/qisql.cpp
    src/qifield.cpp
    src/qisharedquery.cpp
    src/qiqueryrules.cpp
    src/qiexpression.cpp
    src/qiabstractmodel.cpp
    src/qisharedlist.cpp
    src/qiindex.cpp
    src/qistream.cpp
    src/qilistwriter.cpp
    src/qijsonmapper.cpp
    src/qimigrator.cpp
    src/qirecorder.cpp
    src/qivalidation.cpp
    src/qischema.cpp
    src/qilistmodel.cpp)

set(QIVOT_PUBLIC_HEADERS
    src/qierror.h src/qilog.h src/qiclause.h src/qimodelmetainfo.h src/qimodel.h
    src/qiconnection.h src/qibasefield.h src/qisqlstatement.h src/qisqlitestatement.h
    src/qimysqlstatement.h src/qipgstatement.h src/qimssqlstatement.h src/qioraclestatement.h
    src/qiduckdbstatement.h
    src/qiwhere.h src/qisql.h src/qifield.h src/qiforeignkey.h src/qisharedquery.h
    src/qiquery.h src/qiqueryrules.h src/qiexpression.h src/qilist.h src/qiabstractmodel.h
    src/qisharedlist.h src/qiindex.h src/qistream.h src/qilistwriter.h src/qijoin.h
    src/qifieldref.h src/qicolumns.h src/qirelation.h src/qiftsindex.h src/qitransaction.h src/qigadget.h src/qilistmodel.h
    src/qijsonmapper.h src/qimigrator.h src/qirecorder.h src/qirule.h src/qivalidation.h src/qischema.h src/qivot.h)

if(QIVOT_WITH_NETWORK)
    list(APPEND QIVOT_SOURCES src/qijsonrequest.cpp)
    list(APPEND QIVOT_PUBLIC_HEADERS src/qijsonrequest.h)
endif()

# --- The static library ---
add_library(qivot STATIC ${QIVOT_SOURCES})
add_library(Qivot::qivot ALIAS qivot)

target_include_directories(qivot PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include/qivot>)

target_link_libraries(qivot PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql)
if(QIVOT_WITH_NETWORK)
    target_link_libraries(qivot PUBLIC Qt${QT_VERSION_MAJOR}::Network)
endif()

# --- Optional: forms in QML (QiForm + the Qivot.Forms controls) ---
if(QIVOT_WITH_QML)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Qml Quick)
    set(CMAKE_AUTORCC ON)
    add_library(qivotqml STATIC src/qml/qiform.cpp src/qml/qiform.h src/qml/qivotqml.qrc)
    add_library(Qivot::qml ALIAS qivotqml)
    target_include_directories(qivotqml PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/qml>)
    target_link_libraries(qivotqml PUBLIC qivot Qt${QT_VERSION_MAJOR}::Qml Qt${QT_VERSION_MAJOR}::Quick)
    if(NOT MSVC)
        target_compile_options(qivotqml PRIVATE -Wno-invalid-offsetof)
    endif()
endif()

# QiModel uses offsetof on a polymorphic type on purpose (see the reflection
# design); silence the warning to match the qmake build.
if(NOT MSVC)
    target_compile_options(qivot PRIVATE -Wno-invalid-offsetof)
endif()

set_target_properties(qivot PROPERTIES
    VERSION ${PROJECT_VERSION}
    PUBLIC_HEADER "${QIVOT_PUBLIC_HEADERS}")

# --- Install + package config (find_package(Qivot) -> Qivot::qivot) ---
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS qivot
    EXPORT QivotTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qivot)

install(EXPORT QivotTargets
    FILE QivotTargets.cmake
    NAMESPACE Qivot::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Qivot)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/QivotConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/QivotConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Qivot)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/QivotConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/QivotConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/QivotConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Qivot)

if(QIVOT_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
