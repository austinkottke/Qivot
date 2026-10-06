# Builds smoke.cpp against the single header in QIVOT_HPP_DIR.
cmake_minimum_required(VERSION 3.16)
project(qivot_release_smoke LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Sql)

add_executable(smoke smoke.cpp)
target_include_directories(smoke PRIVATE ${QIVOT_HPP_DIR})
target_link_libraries(smoke PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Sql)
