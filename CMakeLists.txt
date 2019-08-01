cmake_minimum_required(VERSION 3.10)


#コンパイラーがC++17に対応しているかチェック
include(CheckCXXCompilerFlag)

enable_language(CXX)

set(CMAKE_CXX_STANDARD 17)
CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)
if(NOT ${COMPILER_SUPPORTS_CXX17})
    message(FATAL_ERROR "${CMAKE_CXX_COMPILER} doesn't support C++17")
endif()

set(CMAKE_CXX_FLAGS "-fmax-errors=10 -ftemplate-backtrace-limit=0 ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g ${CMAKE_CXX_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og -g -D_GLIBCXX_DEBUG -DUSE_FULL_ASSERT")


# Set warning flags
set(CXX_WARNING_FLAGS
        -Wall
        -Wextra
        -Wconversion
        -Wswitch-default
        -Wdisabled-optimization
        -Wformat
        -Winit-self
        -Woverloaded-virtual
        -Wfloat-equal
        -Wno-old-style-cast
        -Wno-pragmas
        -Wno-ignored-attributes
        -Wreturn-type
        )
foreach(FLAG IN LISTS CXX_WARNING_FLAGS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}")
endforeach()


if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()
message("Build type:\n\t${CMAKE_BUILD_TYPE}")


find_package(Boost REQUIRED
    COMPONENTS system thread context coroutine)
if(Boost_FOUND)
    message("Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message("Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
else()
    message(FATAL_ERROR "Boost not Found")
endif()


set(COMMON_DIR "${CMAKE_CURRENT_LIST_DIR}/common")

set(THREAD_SAFE ON CACHE BOOL "Use mutexes to ensure thread safety")
set(JUST_INSTALL_CEREAL ON CACHE BOOL "Don't do anything besides installing the library")

set(CHILDREN
        cereal
        common
        server
        client
        )
foreach(child IN LISTS CHILDREN)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/${child}" "${child}")
endforeach()
