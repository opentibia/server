file(GLOB_RECURSE SRC_LIST *.cpp)
file(GLOB_RECURSE HDR_LIST *.h)

find_package(Boost COMPONENTS thread regex system filesystem REQUIRED)
find_package(GMP)
find_package(MySQL)
find_package(SQLite)
find_package(Lua 5.1)
find_package(LibXML2)

include_directories(${MYSQL_INCLUDE_DIR} ${LUA_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${GMP_INCLUDE_DIR} ${LibXML2_INCLUDE_DIR})
add_executable(${PROJECT_NAME} ${SRC_LIST} ${HDR_LIST})
target_link_libraries(${PROJECT_NAME} ${MYSQL_LIBRARY} ${LUA_LIBRARIES} ${Boost_LIBRARIES} ${GMP_LIBRARY} ${LibXML2_LIBRARIES})
