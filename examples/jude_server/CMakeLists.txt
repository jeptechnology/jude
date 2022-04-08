cmake_minimum_required(VERSION 3.10)

project(jude_server)

find_package(Threads REQUIRED)

include(../../SchemaHelper.cmake)

GenerateModel(ServerSchema ${PROJECT_SOURCE_DIR}/schemas)

add_executable(jude_server
   main.cpp
   ${PROJECT_SOURCE_DIR}/../../src/porting/jude_port_std_c++11.cpp
   )

find_package (Threads)

target_link_libraries(jude_server ServerSchema jude ${CMAKE_THREAD_LIBS_INIT})

