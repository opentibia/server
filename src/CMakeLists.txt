# list all files
file(GLOB_RECURSE SRC_LIST *.cpp)
file(GLOB_RECURSE HDR_LIST *.h)

# find required components
find_package(Boost COMPONENTS thread regex system filesystem REQUIRED)
find_package(GMP REQUIRED)
find_package(LibXML2 REQUIRED)

# include library headers
include_directories(${Boost_INCLUDE_DIRS} ${GMP_INCLUDE_DIR} ${LibXML2_INCLUDE_DIR})

# add executable
add_executable(${PROJECT_NAME} ${SRC_LIST} ${HDR_LIST})

# link libraries
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} ${GMP_LIBRARY} ${LibXML2_LIBRARIES})

# default build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

# compile flags
set(WARNS_FLAGS "-Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-result")
set(CMAKE_CXX_FLAGS                   "${CMAKE_CXX_FLAGS} ${WARNS_FLAGS}")
set(CMAKE_CXX_FLAGS_COMPILESPEED      "-O0")
set(CMAKE_CXX_FLAGS_DEBUG             "-O0 -g")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO    "-O1 -g -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_RELEASE           "-Os")
set(CMAKE_CXX_FLAGS_PERFORMANCE       "-Ofast -mmmx -msse -msse2")

# check if MinGW
if(WIN32 AND MINGW)
    add_definitions(-D__WINDOWS__ -D_WIN32_WINNT=0x0501)
endif()

# choose Lua 5.1 or LuaJIT
if(USE_LUAJIT)
  find_package(LuaJIT 2.0.3 REQUIRED)
  include_directories(${LUAJIT_INCLUDE_DIR})
  target_link_libraries(${PROJECT_NAME} ${LUAJIT_LIBRARY})
else()
  find_package(Lua 5.1 REQUIRED)
  include_directories(${LUA_INCLUDE_DIR})
  target_link_libraries(${PROJECT_NAME} ${LUA_LIBRARIES})
endif()

# link pthread
if(UNIX)
  target_link_libraries(${PROJECT_NAME} pthread)
endif()

# link winsock
if(WIN32)
  target_link_libraries(${PROJECT_NAME} ws2_32 mswsock)
endif()

# display build type
message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})

if(USE_MYSQL)
  add_definitions(-D__USE_MYSQL__)
  find_package(MySQL REQUIRED)
  include_directories(${MYSQL_INCLUDE_DIR})
  target_link_libraries(${PROJECT_NAME} ${MYSQL_LIBRARY})
endif()

if(USE_SQLITE)
  add_definitions(-D__USE_SQLITE__)
  find_package(SQLite REQUIRED)
  include_directories(${SQLITE_INCLUDE_DIR})
  target_link_libraries(${PROJECT_NAME} ${SQLITE_LIBRARY})
endif()

if(USE_ODBC)
  add_definitions(-D__USE_ODBC__)
  find_package(ODBC REQUIRED)
  include_directories(${ODBC_INCLUDE_DIR})
  target_link_libraries(${PROJECT_NAME} ${ODBC_LIBRARIES})
endif()

if(USE_PQSQL)
  add_definitions(-D__USE_PQSQL__)
  find_package(PostgreSQL REQUIRED)
  include_directories(${POSTGRESQL_INCLUDE_DIR})
  target_link_libraries(${PROJECT_NAME} ${POSTGRESQL_LIBRARIES})
endif()
