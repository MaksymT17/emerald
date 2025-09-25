cmake_minimum_required(VERSION 3.20)
project(emerald CXX)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES main.cpp)

find_package (Threads)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

include_directories(${GTK3_INCLUDE_DIRS} ".")

add_subdirectory(silber)

link_directories("${CMAKE_BINARY_DIR}/silber/")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/silber")

link_directories(${GTK3_LIBRARY_DIRS})

add_executable(emerald main.cpp FileProvider.hpp)
include_directories(${GTK3_INCLUDE_DIRS})
target_link_libraries(emerald silber-1.2 ${GTK3_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})