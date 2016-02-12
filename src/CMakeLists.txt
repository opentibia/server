# list all files
file(GLOB_RECURSE SRC_LIST *.cpp)
file(GLOB_RECURSE HDR_LIST *.h)

# static linking
if(USE_STATIC_LIBS)
  SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
  SET(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")
endif()

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

# libxml2 if linked statically requires zlib + iconv + lzma on Unix
if(USE_STATIC_LIBS)
  find_package(ZLIB REQUIRED)
  find_package(Iconv REQUIRED)
  target_link_libraries(${PROJECT_NAME} ${ZLIB_LIBRARIES} ${ICONV_LIBRARIES})
  
  if(UNIX)
    find_package(LZMA REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${LZMA_LIBRARIES})
  endif()
  
  # notify cmake about static build
  set_target_properties(${PROJECT_NAME} PROPERTIES LINK_SEARCH_START_STATIC ON)
  set_target_properties(${PROJECT_NAME} PROPERTIES LINK_SEARCH_END_STATIC ON)
  
  # notify source code about static build
  remove_definitions(-D__STATIC__)
  add_definitions(-D__STATIC__)
endif()

# server diagnostic
if(USE_DIAGNOSTIC)
  remove_definitions(-D__ENABLE_SERVER_DIAGNOSTIC__)
  add_definitions(-D__ENABLE_SERVER_DIAGNOSTIC__)
endif()

# skull system
if(USE_SKULLSYSTEM)
  remove_definitions(-D__SKULLSYSTEM__)
  add_definitions(-D__SKULLSYSTEM__)
endif()

# default build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

# compile flags
set(WARNING_FLAGS "-Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-result")
set(CMAKE_CXX_FLAGS                   "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS}")
set(CMAKE_CXX_FLAGS_COMPILESPEED      "-O0")
set(CMAKE_CXX_FLAGS_DEBUG             "-O0 -g")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO    "-O1 -g -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_RELEASE           "-Os")
set(CMAKE_CXX_FLAGS_PERFORMANCE       "-Ofast -mmmx -msse -msse2")

# add debug definition
if(CMAKE_BUILD_TYPE EQUAL "Debug")
  remove_definitions(-D__DEBUG__ -D__DEBUG_NET_DETAIL__ -D__DEBUG__MOVESYS__ -D__DEBUG_CRITICALSECTION__ -D__DEBUG_HOUSES__ -D__DEBUG_PLAYERS__ -D__DEBUG_SCHEDULER__ -D__DEBUG_SCRIPT_ENVIRONMENT_OBJECTMAP__)
  add_definitions(-DD__DEBUG__ -D__DEBUG_NET_DETAIL__ -D__DEBUG__MOVESYS__ -D__DEBUG_CRITICALSECTION__ -D__DEBUG_HOUSES__ -D__DEBUG_PLAYERS__ -D__DEBUG_SCHEDULER__ -D__DEBUG_SCRIPT_ENVIRONMENT_OBJECTMAP__)
endif()

# check if MinGW
if(WIN32 AND MINGW)
  remove_definitions(-D__WINDOWS__ -D_WIN32_WINNT)
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

if(USE_PGSQL)
  add_definitions(-D__USE_PGSQL__)
  find_package(PostgreSQL REQUIRED)
  include_directories(${POSTGRESQL_INCLUDE_DIR})
  target_link_libraries(${PROJECT_NAME} ${POSTGRESQL_LIBRARIES})
endif()

# link pthread and dl
if(UNIX)
  find_package(Threads)
  target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} dl)
endif()

# link winsock
if(WIN32)
  target_link_libraries(${PROJECT_NAME} ws2_32 mswsock)
endif()

# display build type
message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})
