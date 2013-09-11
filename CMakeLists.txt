cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(otserv CXX)

# Additional CMake modules
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(VERSION_MAJOR 0)
set(VERSION_MINOR 7)
set(VERSION_MICRO 0)
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_MICRO}")

add_subdirectory(src)
