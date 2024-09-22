cmake_minimum_required(VERSION 3.20)
project(am_gui CXX)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES main.cpp)

find_package (Threads)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk3)

include_directories(${GTK_INCLUDE_DIRS} ".")

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

elseif(WIN32)
    set(LIB_URL "https://github.com/MaksymT17/silber/releases/download/1.1/windows_debug.zip")
    set(LIB_NAME "windows_debug.zip")
    set(LIB_PATH "${CMAKE_BINARY_DIR}/${LIB_NAME}")  # Full path including filename
    file(DOWNLOAD ${LIB_URL} ${LIB_PATH})
    execute_process(COMMAND powershell -Command "Expand-Archive -Path ${LIB_PATH} -DestinationPath '${CMAKE_BINARY_DIR}'")

else()
#print unsupported OS
endif()

file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/silber/lib")
file(COPY "${CMAKE_BINARY_DIR}/lib" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/silber/")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/silber/include")
file(COPY "${CMAKE_BINARY_DIR}/include" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/silber/")

link_directories("${CMAKE_CURRENT_SOURCE_DIR}/silber/lib")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/silber/include")

link_directories(${GTK_LIBRARY_DIRS})
#add_subdirectory(sh_mem)
add_executable(am_gui main.cpp FileProvider.hpp )
 
target_link_libraries(am_gui silber-1.1 ${GTK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})