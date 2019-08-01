cmake_minimum_required(VERSION 3.10)

project(server CXX)


file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

add_executable(server "${SOURCES}")
target_link_libraries(server common)
