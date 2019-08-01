cmake_minimum_required(VERSION 3.10)

project(common CXX)


find_package(Threads REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PULSE REQUIRED libpulse)

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

add_library(common INTERFACE)
target_include_directories(
    common SYSTEM
    INTERFACE "${PULSE_INCLUDE_DIRS}"
)
target_include_directories(
    common
    INTERFACE "${CMAKE_CURRENT_LIST_DIR}"
)
target_compile_options(common INTERFACE "${PULSE_CFLAGS}")
target_link_libraries(
    common
    INTERFACE ${Boost_SYSTEM_LIBRARY}
    INTERFACE "${PULSE_LIBRARIES}"
    INTERFACE Threads::Threads
    INTERFACE cereal
)
target_sources(
    common
    INTERFACE "${SOURCES}"
)
