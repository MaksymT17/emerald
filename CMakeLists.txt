cmake_minimum_required(VERSION 3.20)
project(emerald CXX)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES main.cpp)

find_package (Threads)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

include_directories(${GTK3_INCLUDE_DIRS} ".")

# Unpack the archive based on the platform
if(UNIX AND NOT APPLE)
    set(LIB_URL "https://github.com/MaksymT17/silber/releases/download/1.1/linux_release.zip")
    set(LIB_NAME "linux_release.zip")
    set(LIB_PATH "${CMAKE_BINARY_DIR}/${LIB_NAME}")  # Full path including filename
    file(DOWNLOAD ${LIB_URL} ${LIB_PATH})
    execute_process(COMMAND unzip ${LIB_PATH} -d ${CMAKE_BINARY_DIR})

elseif(APPLE)
    set(LIB_URL "https://github.com/MaksymT17/silber/releases/download/1.1/macos_release.zip")
    set(LIB_NAME "macos_release.zip")
    set(LIB_PATH "${CMAKE_BINARY_DIR}/${LIB_NAME}")  # Full path including filename
    file(DOWNLOAD ${LIB_URL} ${LIB_PATH})
    execute_process(COMMAND unzip ${LIB_PATH} -d ${CMAKE_BINARY_DIR})
else()
    message(ERROR "Application has been tested on MacOS and Linux(briefly)")

endif()

file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/silber/lib")
file(COPY "${CMAKE_BINARY_DIR}/lib" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/silber/")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/silber/include")
file(COPY "${CMAKE_BINARY_DIR}/include" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/silber/")

link_directories("${CMAKE_CURRENT_SOURCE_DIR}/silber/lib")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/silber/include")

link_directories(${GTK3_LIBRARY_DIRS})
#add_subdirectory(sh_mem)
add_executable(am_gui main.cpp FileProvider.hpp )
include_directories(${GTK3_INCLUDE_DIRS} )
target_link_libraries(am_gui silber-1.1 ${GTK3_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})