cmake_minimum_required(VERSION 3.10)

project(client CXX)


file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

add_executable(client "${SOURCES}")
target_link_libraries(client common)
