
cmake_minimum_required(VERSION 3.1.0)
project(HDK-Logger)

#list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#
# Dependencies
#

find_package(Threads REQUIRED)

#
# Third-party libraries
#
add_subdirectory(vendor)

if(NOT TARGET hidapi)
    message(FATAL_ERROR "This app doesn't do anything without HIDAPI - fix the errors above!")
endif()

add_executable(hdk-logger HDK-Logger.cpp)
target_link_libraries(hdk-logger PRIVATE hidapi)
set_property(TARGET hdk-logger PROPERTY CXX_STANDARD 11)
