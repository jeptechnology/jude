cmake_minimum_required(VERSION 3.10)

project(jude)

option(JUDE_UNIT_TESTS "Build jude unit tests" ON)
option(JUDE_CLI        "Build jude cli" ON)
option(JUDE_EXAMPLES   "Build jude examples" OFF)
option(JUDE_USE_STDLIB "Build jude library with the std:: platform implementation" ON)

if (JUDE_USE_STDLIB)
   add_definitions(-DJUDE_USE_STDLIB)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

set(jude_include_dir "${CMAKE_CURRENT_LIST_DIR}/include")

if (MSVC)
   set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Ox /MTd")
   set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Od /MTd")
   set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Ox /MTd")
   set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /MTd")

   # VS 2017 removed tr1
   add_definitions(-DGTEST_HAS_TR1_TUPLE=0)
   add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
else()
   set(COMMON_FLAGS "-g -O0 -Wall -Wextra -Wconversion -Wswitch-default -Wswitch-enum -Wunreachable-code -Wwrite-strings -Wcast-align -Wshadow -Wundef")
endif()

add_subdirectory(src)

if (JUDE_UNIT_TESTS)
   add_subdirectory(tests)
endif()

if (JUDE_CLI)
   add_subdirectory(cli)
endif()

if (JUDE_EXAMPLES)
   add_subdirectory(examples/jude_server)
endif()
