# List all files
file(GLOB_RECURSE SRC_LIST *.cpp)
file(GLOB_RECURSE HDR_LIST *.h)

# Find required components
find_package(Boost COMPONENTS thread regex system filesystem REQUIRED)
find_package(GMP REQUIRED)
find_package(LibXML2 REQUIRED)

# Include library headers
include_directories(${Boost_INCLUDE_DIRS} ${GMP_INCLUDE_DIR} ${LibXML2_INCLUDE_DIR})

# Add executable
add_executable(${PROJECT_NAME} ${SRC_LIST} ${HDR_LIST})

# Link libraries
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} ${GMP_LIBRARY} ${LibXML2_LIBRARIES})

# Choose Lua 5.1 or LuaJIT
if(USE_LUAJIT)
  find_package(LuaJIT 2.0.3 REQUIRED)
  include_directories(${LUAJIT_INCLUDE_DIR})
  target_link_libraries(${PROJECT_NAME} ${LUAJIT_LIBRARY})
else()
  find_package(Lua 5.1 REQUIRED)
  include_directories(${LUA_INCLUDE_DIR})
  target_link_libraries(${PROJECT_NAME} ${LUA_LIBRARIES})
endif()

# Link pthread
if(UNIX)
  target_link_libraries(${PROJECT_NAME} pthread)
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

if(USE_PQSQL)
  add_definitions(-D__USE_PQSQL__)
  find_package(PostgreSQL REQUIRED)
  include_directories(${POSTGRESQL_INCLUDE_DIR})
  target_link_libraries(${PROJECT_NAME} ${POSTGRESQL_LIBRARIES})
endif()
