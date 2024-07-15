cmake_minimum_required(VERSION 3.20)
project(am_gui CXX)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES main.cpp)

find_package (Threads)
find_package (SQLite3)
find_package(JPEG REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED gtk+-3.0)

include_directories(${GTK_INCLUDE_DIRS} ${SQLite3_INCLUDE_DIRS} ${JPEG_INCLUDE_DIR} ./aquamarine )

link_directories(${GTK_LIBRARY_DIRS} ./aquamarine/build)

add_executable(am_gui main.cpp FileProvider.hpp)
target_link_libraries(am_gui ${GTK_LIBRARIES} aquamarine_lib ${CMAKE_THREAD_LIBS_INIT} ${SQLite3_LIBRARIES} ${JPEG_LIBRARIES})