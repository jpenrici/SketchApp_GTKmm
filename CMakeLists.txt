cmake_minimum_required(VERSION 3.5)

project(Sketch_GTKmm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/main.cpp
    src/sketchApp.h
    src/sketchApp.cpp
    src/sketchWindow.h
    src/sketchWindow.cpp
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-4.0)

add_executable(${PROJECT_NAME} ${SOURCES})

link_directories(${GTKMM_LIBRARY_DIRS})
include_directories(include ${GTKMM_INCLUDE_DIRS})

target_link_libraries (
    ${PROJECT_NAME} PRIVATE
    ${GTKMM_LIBRARIES}
)

install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
