cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(otserv)

# Additional CMake modules
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(VERSION_MAJOR 0)
set(VERSION_MINOR 7)
set(VERSION_MICRO 0)
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_MICRO}")

# Use MySQL
set(CMAKE_CXX_FLAGS             "-D__USE_MYSQL__")

# Make sure the compiler can compile C++11 code
include(FindCXX11)

# Show all warnings and make them as errors
#set(CMAKE_CXX_FLAGS             "-Wall -Werror ${CMAKE_CXX_FLAGS}")

add_subdirectory(src)
