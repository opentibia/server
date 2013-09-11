file(GLOB_RECURSE SRC_LIST *.cpp)
file(GLOB_RECURSE HDR_LIST *.h)

find_package(Boost COMPONENTS thread regex system filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} ${SRC_LIST} ${HDR_LIST})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})