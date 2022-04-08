cmake_minimum_required(VERSION 3.2)

project(jude_test)

# CTest
enable_testing()

include_directories(SYSTEM
   ${jude_SOURCE_DIR}/include
   )

include(../SchemaHelper.cmake)

GenerateModel(TestSchema ${PROJECT_SOURCE_DIR}/schemas)

file(GLOB_RECURSE TestSources 
   ${PROJECT_SOURCE_DIR}/test_*.cpp
   )

add_executable(jude_test
   
   ${TestSources}

   ${PROJECT_SOURCE_DIR}/../src/porting/jude_port_std_c++11.cpp
   ${PROJECT_SOURCE_DIR}/porting/jude_test_porting.cpp   
   )

find_package (Threads)

include(gtest.cmake)

target_link_libraries(jude_test gmock_main gmock TestSchema jude ${CMAKE_THREAD_LIBS_INIT})
