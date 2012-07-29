CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(otserv CXX)

# CMake 2.6 compatibility
IF(${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 6)
    GET_FILENAME_COMPONENT(CMAKE_CURRENT_LIST_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)
ENDIF(${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 6)

# Additional CMake modules
SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

# Choose default build type if not specified
IF(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE RelWithDebInfo)
ENDIF(NOT CMAKE_BUILD_TYPE)
MESSAGE(STATUS "BUILD TYPE: " ${CMAKE_BUILD_TYPE})

SET(SERVER_SOURCES "")
SET(SERVER_HEADERS "")
SET(SERVER_LIBRARIES "")
SET(SERVER_DEFINITIONS "")
SET(SERVER_INCLUDE_DIRS "")

SET(VERSION "0.7.0")
SET(VERSION_MAJOR 0)
SET(VERSION_MINOR 7)
SET(VERSION_MICRO 0)

# Select the database drivers
OPTION(USE_SQLITE "build the server with SQLite database support" ON)
OPTION(USE_MYSQL "build the server with MySQL database support" OFF)
OPTION(USE_ODBC "build the server with ODBC database support" OFF)
OPTION(USE_POSTGRE "build the server with PostgreSQL database support" OFF)
OPTION(USE_LUAJIT "build the server with lua jit" OFF)

# Gives priority to static libraries (links faster)
SET(USE_STATIC_LIBS ON)

# Boost
SET(Boost_USE_MULTITHREADED ON)
IF(MINGW)
	SET(Boost_THREADAPI win32)
	SET(SERVER_DEFINITIONS ${SERVER_DEFINITIONS} -DBOOST_THREAD_USE_LIB)
ENDIF()
FIND_PACKAGE(Boost COMPONENTS thread regex system filesystem REQUIRED)
SET(SERVER_LIBRARIES ${SERVER_LIBRARIES} ${Boost_LIBRARIES})
SET(SERVER_INCLUDE_DIRS ${SERVER_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})

# LibXML2
FIND_PACKAGE(LibXml2 REQUIRED)
SET(SERVER_LIBRARIES ${SERVER_LIBRARIES} ${LIBXML2_LIBRARIES})
SET(SERVER_INCLUDE_DIRS ${SERVER_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIR})

# GMP
FIND_PACKAGE(GMP REQUIRED)
SET(SERVER_LIBRARIES ${SERVER_LIBRARIES} ${GMP_LIBRARY})
SET(SERVER_INCLUDE_DIRS ${SERVER_INCLUDE_DIRS} ${GMP_INCLUDE_DIR})

# Lua
IF(USE_LUAJIT)
	FIND_PACKAGE(LuaJIT REQUIRED)
	SET(SERVER_INCLUDE_DIRS ${SERVER_INCLUDE_DIRS} ${LUAJIT_INCLUDE_DIR})
	SET(SERVER_LIBRARIES ${SERVER_LIBRARIES} ${LUAJIT_LIBRARY})
	MESSAGE(STATUS "LuaJIT: ON")
ELSE()
	FIND_PACKAGE(Lua51 REQUIRED)
	SET(SERVER_INCLUDE_DIRS ${SERVER_INCLUDE_DIRS} ${LUA_INCLUDE_DIR})
	SET(SERVER_LIBRARIES ${SERVER_LIBRARIES} ${LUA_LIBRARY})
	MESSAGE(STATUS "LuaJIT: OFF")
ENDIF()

# SQLite, enabled by default
IF(USE_SQLITE)
	FIND_PACKAGE(SQLite REQUIRED)
	SET(SERVER_SOURCES ${SERVER_SOURCES} Source/database_driver_sqlite.cpp)
	SET(SERVER_HEADERS ${SERVER_HEADERS} Source/database_driver_sqlite.h)
	SET(SERVER_DEFINITIONS ${SERVER_DEFINITIONS} -D__USE_SQLITE__)
	SET(SERVER_LIBRARIES ${SERVER_LIBRARIES} ${SQLITE_LIBRARY})
	SET(SERVER_INCLUDE_DIRS ${SERVER_INCLUDE_DIRS} ${SQLITE_INCLUDE_DIR})
	MESSAGE(STATUS "SQLite: ON")
ENDIF()

# MySQL
IF(USE_MYSQL)
	FIND_PACKAGE(MySQL REQUIRED)
	SET(SERVER_SOURCES ${SERVER_SOURCES} Source/database_driver_mysql.cpp)
	SET(SERVER_HEADERS ${SERVER_HEADERS} Source/database_driver_mysql.h)
	SET(SERVER_DEFINITIONS ${SERVER_DEFINITIONS} -D__USE_MYSQL__)
	SET(SERVER_LIBRARIES ${SERVER_LIBRARIES} ${MYSQL_LIBRARY})
	SET(SERVER_INCLUDE_DIRS ${SERVER_INCLUDE_DIRS} ${MYSQL_INCLUDE_DIR})
	MESSAGE(STATUS "MySQL: ON")
ENDIF()

# PostgreSQL
IF(USE_POSTGRE)
	FIND_PACKAGE(PostgreSQL REQUIRED)
	SET(SERVER_SOURCES ${SERVER_SOURCES} Source/database_driver_pgsql.cpp)
	SET(SERVER_HEADERS ${SERVER_HEADERS} Source/database_driver_pgsql.h)
	SET(SERVER_DEFINITIONS ${SERVER_DEFINITIONS} -D__USE_PGSQL__)
	SET(SERVER_LIBRARIES ${SERVER_LIBRARIES} ${POSTGRESQL_LIBRARY})
	SET(SERVER_INCLUDE_DIRS ${SERVER_INCLUDE_DIRS} ${POSTGRESQL_INCLUDE_DIR})
	MESSAGE(STATUS "PostgreSQL: ON")
ENDIF()
# ODBC
IF(USE_ODBC)
	SET(SERVER_SOURCES ${SERVER_SOURCES} Source/database_driver_odbc.cpp)
	SET(SERVER_HEADERS ${SERVER_HEADERS} Source/database_driver_odbc.h)
	SET(SERVER_DEFINITIONS ${SERVER_DEFINITIONS} -D__USE_ODBC__)
	SET(SERVER_LIBRARIES ${SERVER_LIBRARIES} ${ODBC_LIBRARY})
	SET(SERVER_INCLUDE_DIRS ${SERVER_INCLUDE_DIRS} ${ODBC_INCLUDE_DIR})
	MESSAGE(STATUS "ODBC: ON")
ENDIF()

# Compiler flags
IF(CMAKE_COMPILER_IS_GNUCXX)
	SET(CMAKE_CXX_FLAGS                "-Wall")
	SET(CMAKE_CXX_FLAGS_DEBUG          "-O0 -g")
	SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O1 -g")
	SET(CMAKE_CXX_FLAGS_RELEASE        "-O2")
	SET(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os")
	IF(WIN32)
		SET(SERVER_LIBRARIES ${SERVER_LIBRARIES} ws2_32 mswsock)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mthreads")
	ELSE(WIN32)
		SET(SERVER_LIBRARIES ${SERVER_LIBRARIES} dl)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
	ENDIF(WIN32)
ELSEIF(MSVC)
	LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
	ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
ENDIF()

IF(WIN32)
	SET(SERVER_DEFINITIONS ${SERVER_DEFINITIONS} -D__WINDOWS__)
ENDIF()

# Sources
SET(SERVER_SOURCES ${SERVER_SOURCES}
	Source/actor.cpp
	Source/admin.cpp
	Source/allocator.cpp
	Source/ban.cpp
	Source/beds.cpp
	Source/chat.cpp
	Source/combat.cpp
	Source/condition.cpp
	Source/condition_attributes.cpp
	Source/configmanager.cpp
	Source/connection.cpp
	Source/const.cpp
	Source/container.cpp
	Source/creature.cpp
	Source/creature_manager.cpp
	Source/creature_type.cpp
	Source/cylinder.cpp
	Source/database_driver.cpp
	Source/depot.cpp
	Source/enums.cpp
	Source/exception.cpp
	Source/fileloader.cpp
	Source/game.cpp
	Source/house.cpp
	Source/housetile.cpp
	Source/http_request.cpp
	Source/ioaccount.cpp
	Source/iomapotbm.cpp
	Source/iomapserialize.cpp
	Source/ioplayer.cpp
	Source/item.cpp
	Source/item_attributes.cpp
	Source/items.cpp
	Source/logger.cpp
	Source/lua_manager.cpp
	Source/map.cpp
	Source/md5.cpp
	Source/networkmessage.cpp
	Source/otserv.cpp
	Source/outputmessage.cpp
	Source/party.cpp
	Source/player.cpp
	Source/position.cpp
	Source/protocol.cpp
	Source/protocolgame.cpp
	Source/protocollogin.cpp
	Source/protocolold.cpp
	Source/rsa.cpp
	Source/scheduler.cpp
	Source/script_environment.cpp
	Source/script_event.cpp
	Source/script_functions.cpp
	Source/script_listener.cpp
	Source/script_manager.cpp
	Source/server.cpp
	Source/sha1.cpp
	Source/spawn.cpp
	Source/status.cpp
	Source/tasks.cpp
	Source/teleport.cpp
	Source/thing.cpp
	Source/tile.cpp
	Source/tools.cpp
	Source/trashholder.cpp
	Source/vocation.cpp
	Source/waitlist.cpp
)

# Headers
SET(SERVER_HEADERS ${SERVER_HEADERS}
	Source/account.h
	Source/actor.h
	Source/admin.h
	Source/allocator.h
	Source/ban.h
	Source/beds.h
	Source/boost_common.h
	Source/chat.h
	Source/classes.h
	Source/combat.h
	Source/condition.h
	Source/condition_attributes.h
	Source/configmanager.h
	Source/connection.h
	Source/const.h
	Source/container.h
	Source/creature.h
	Source/creature_manager.h
	Source/creature_type.h
	Source/cylinder.h
	Source/database_driver.h
	Source/definitions.h # We need to get rid of this
	Source/depot.h
	Source/enum.h
	Source/enums.h
	Source/exception.h
	Source/fileloader.h
	Source/game.h
	Source/house.h
	Source/housetile.h
	Source/http_request.h
	Source/ioaccount.h
	Source/iomap.h
	Source/iomapotbm.h
	Source/iomapserialize.h
	Source/ioplayer.h
	Source/item.h
	Source/item_attributes.h
	Source/itemloader.h
	Source/items.h
	Source/logger.h
	Source/lua_manager.h
	Source/map.h
	Source/md5.h
	Source/networkmessage.h
	Source/otsystem.h
	Source/outfit.h
	Source/outputmessage.h
	Source/party.h
	Source/player.h
	Source/position.h
	Source/protocol.h
	Source/protocolgame.h
	Source/protocollogin.h
	Source/protocolold.h
	Source/rsa.h
	Source/scheduler.h
	Source/script_environment.h
	Source/script_event.h
	Source/script_listener.h
	Source/script_manager.h
	Source/server.h
	Source/sha1.h
	Source/shared_ptr.h
	Source/spawn.h
	Source/status.h
	Source/tasks.h
	Source/teleport.h
	Source/templates.h
	Source/thing.h
	Source/tile.h
	Source/tools.h
	Source/town.h
	Source/trashholder.h
	Source/vocation.h
	Source/waitlist.h
	Source/waypoints.h
)

INCLUDE_DIRECTORIES(${SERVER_INCLUDE_DIRS})
ADD_DEFINITIONS(${SERVER_DEFINITIONS})
ADD_EXECUTABLE(${PROJECT_NAME} ${SERVER_SOURCES} ${SERVER_HEADERS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${SERVER_LIBRARIES})
