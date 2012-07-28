# This still needs to be checked
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(otserv CXX)

SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

SET(VERSION "0.7.0")
SET(VERSION_MAJOR 0)
SET(VERSION_MINOR 7)
SET(VERSION_MICRO 0)

# Select the database drivers
OPTION(USE_MYSQL "build the server with MySQL database support" OFF)
OPTION(USE_ODBC "build the server with ODBC database support" OFF)
OPTION(USE_POSTGRE "build the server with PostgreSQL database support" OFF)
OPTION(USE_SQLITE "build the server with SQLite database support" OFF)

FIND_PACKAGE(Boost REQUIRED COMPONENTS
	thread
	regex
	system
	)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
	
FIND_PACKAGE(LibXml2 REQUIRED)
INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})

FIND_PACKAGE(GMP REQUIRED)
INCLUDE_DIRECTORIES(${GMP_INCLUDE_DIR})

FIND_PACKAGE(Lua51 REQUIRED)
INCLUDE_DIRECTORIES(${LUA_INCLUDE_DIR})

# Find database packages
IF(USE_MYSQL)
	FIND_PACKAGE(MySql REQUIRED)
ENDIF()

IF(USE_POSTGRE)
	FIND_PACKAGE(PostgreSQL REQUIRED)
ENDIF()

IF(USE_SQLITE)
	FIND_PACKAGE(SQLite REQUIRED)
ENDIF()

SET(SERVER_SRC
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
	
SET(SERVER_HDR
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
	
IF(USE_MYSQL)
	SET(SERVER_SRC ${SERVER_SRC} Source/database_driver_mysql.cpp)
	SET(SERVER_HDR ${SERVER_HDR} Source/database_driver_mysql.h)
	ADD_DEFINITIONS(-D__USE_MYSQL__)
ENDIF()

IF(USE_ODBC)
	SET(SERVER_SRC ${SERVER_SRC} Source/database_driver_odbc.cpp)
	SET(SERVER_HDR ${SERVER_HDR} Source/database_driver_odbc.h)
	ADD_DEFINITIONS(-D__USE_ODBC__)
ENDIF()

IF(USE_PGSQL)
	SET(SERVER_SRC ${SERVER_SRC} Source/database_driver_pgsql.cpp)
	SET(SERVER_HDR ${SERVER_HDR} Source/database_driver_pgsql.h)
	ADD_DEFINITIONS(-D__USE_PGSQL__)
ENDIF()

IF(USE_SQLITE)
	SET(SERVER_SRC ${SERVER_SRC} Source/database_driver_sqlite.cpp)
	SET(SERVER_HDR ${SERVER_HDR} Source/database_driver_sqlite.h)
	ADD_DEFINITIONS(-D__USE_SQLITE__)
ENDIF()

# MVSC supports auto-linking
IF(MSVC)
	LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
	ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
ENDIF()

ADD_EXECUTABLE(${PROJECT_NAME} ${SERVER_SRC} ${SERVER_HDR})
TARGET_LINK_LIBRARIES(${PROJECT_NAME}
	${Boost_LIBRARIES}
	${LIBXML2_LIBRARIES}
	${LUA_LIBRARIES}
	${GMP_LIBRARIES}
	)
	
