cmake_minimum_required(VERSION 3.10)

project(jude_cli)

find_package(Threads REQUIRED)

add_executable(jude_cli
   main.cpp
   RestApiClient.cpp
   RestApiClient.h
   JsonPrettyPrinter.cpp
   JsonPrettyPrinter.h
   linenoise.hpp
   )

target_include_directories(jude_cli PUBLIC
   ${PROJECT_SOURCE_DIR}/../include
)

find_package (Threads)

target_link_libraries(jude_cli ${CMAKE_THREAD_LIBS_INIT})
