cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ESP32_IDF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fuse-cxa-atexit")
