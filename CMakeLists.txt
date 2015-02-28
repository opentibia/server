cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(otserv)

# Additional CMake modules
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Revscriptsys2
set(VERSION "0.7.0")

# Make sure the compiler can compile C++11 code
include(FindCXX11)

# Options
option(USE_MYSQL "Use MySQL" OFF)
option(USE_SQLITE "Use SQLite" OFF)
option(USE_ODBC "Use ODBC" OFF)
option(USE_PGSQL "Use PGSQL" OFF)
option(USE_LUAJIT "Use LuaJIT" OFF)
option(USE_DIAGNOSTIC "Use server diagnostic" OFF)
option(USE_SKULLSYSTEM "Skull system" ON)
option(USE_STATIC_LIBS "Static linking" OFF)

# Status
message(STATUS "MySQL: " ${USE_MYSQL})
message(STATUS "SQLite: " ${USE_SQLITE})
message(STATUS "ODBC: " ${USE_ODBC})
message(STATUS "PGSQL: " ${USE_PGSQL})
message(STATUS "LuaJIT: " ${USE_LUAJIT})

message(STATUS "Server diagnostic: " ${USE_DIAGNOSTIC})
message(STATUS "Skull system: " ${USE_SKULLSYSTEM})

message(STATUS "Static libraries: " ${USE_STATIC_LIBS})

# Make sure at least one database driver is selected
if(NOT USE_MYSQL AND NOT USE_SQLITE AND NOT USE_ODBC AND NOT USE_PGSQL)
  message(FATAL_ERROR "At least one database driver has to be selected.")
endif()

# Sources
add_subdirectory(src)
