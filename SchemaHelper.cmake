cmake_minimum_required(VERSION 3.10)

set(GENERATOR_DIR ${CMAKE_CURRENT_LIST_DIR})  

macro(GenerateModelImpl ModelName ModelPath AutogenSuffix Options)
   file(GLOB ${ModelName}YamlFiles ${ModelPath}/*.yaml)

   message(STATUS "Adding ${ModelName} model to build from path: ${ModelPath}")

   foreach(YamlFile ${${ModelName}YamlFiles})
      get_filename_component(SchemaName ${YamlFile} NAME_WE)
      message(STATUS "Adding schema: ${SchemaName} to build")
      add_custom_command(
         OUTPUT
            ${PROJECT_SOURCE_DIR}/autogen${AutogenSuffix}/${SchemaName}.model.c
         DEPENDS
            ${YamlFile} ${GENERATOR_DIR}/generator/jude_generator.py
         WORKING_DIRECTORY
            ${PROJECT_SOURCE_DIR}/schemas
         COMMAND
            #  Call this every time you touch yaml files
            python3 ${GENERATOR_DIR}/generator/jude_generator.py ${Options} ${YamlFile} ${PROJECT_SOURCE_DIR}/autogen${AutogenSuffix}
         )

      # Initial schema file generation
      execute_process(COMMAND python3 ${GENERATOR_DIR}/generator/jude_generator.py ${Options} ${YamlFile} ${PROJECT_SOURCE_DIR}/autogen${AutogenSuffix})

   endforeach()

   set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${PROJECT_SOURCE_DIR}/autogen${AutogenSuffix}")

   file(GLOB_RECURSE ${ModelName}Sources
      ${${ModelName}YamlFiles}
      ${PROJECT_SOURCE_DIR}/autogen${AutogenSuffix}/*.h
      ${PROJECT_SOURCE_DIR}/autogen${AutogenSuffix}/*.c
      ${PROJECT_SOURCE_DIR}/autogen${AutogenSuffix}/*.cpp
   )
   
   add_library(${ModelName} STATIC ${${ModelName}Sources})

   target_include_directories(${ModelName} PUBLIC 
      ${PROJECT_SOURCE_DIR}
      ${PROJECT_SOURCE_DIR}/autogen
      ${PROJECT_SOURCE_DIR}/autogen${AutogenSuffix}
   )

   target_link_libraries(${ModelName} PUBLIC jude)

endmacro()

macro(GenerateModel ModelName ModelPath)
    GenerateModelImpl(${ModelName} ${ModelPath} "" "")
endmacro()

macro(GenerateModelLegacy ModelName ModelPath)
    GenerateModelImpl(${ModelName} ${ModelPath} "/Protobuf" "--legacy")
endmacro()
