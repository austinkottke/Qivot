cmake_minimum_required(VERSION 3.16)
project(MyQivotApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Find required packages
find_package(Qt6 COMPONENTS Core Sql REQUIRED)
find_package(Qivot REQUIRED)

# Add source files
add_executable(myapp
    src/main.cpp
)

# Link libraries
target_link_libraries(myapp PRIVATE
    Qt6::Core
    Qt6::Sql
    Qivot::qivot
)

# Include directories
target_include_directories(myapp PRIVATE src)
