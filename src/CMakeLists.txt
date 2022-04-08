cmake_minimum_required(VERSION 3.2)

file(GLOB IncludeFiles ../include/**/*.h)

add_library(jude 
   ${IncludeFiles}
   core/c/jude_bitfield.c
   core/c/jude_common.c
   core/c/jude_decode_binary.c
   core/c/jude_debug.c
   core/c/jude_decode_binary.c
   core/c/jude_decode_json.c
   core/c/jude_decode_raw.c
   core/c/jude_decode.c
   core/c/jude_encode_binary.c
   core/c/jude_encode_json.c
   core/c/jude_encode_raw.c
   core/c/jude_encode.c
   core/c/jude_enum.c
   core/c/jude_field.c
   core/c/jude_filter.c
   core/c/jude_internal.c
   core/c/jude_iterator.c
   core/c/jude_json_schema.c
   core/c/jude_notification_queue.c
   core/c/jude_object.c
   core/c/jude_rtti.c
   core/c/jude_stream.c
   core/cpp/AccessControl.cpp
   core/cpp/AtomicArray.cpp
   core/cpp/BitMask.cpp
   core/cpp/BytesArray.cpp
   core/cpp/FieldMask.cpp
   core/cpp/NotifyQueue.cpp
   core/cpp/Object.cpp
   core/cpp/ObjectArray.cpp
   core/cpp/Options.cpp
   core/cpp/RestApiInterface.cpp
   core/cpp/RestfulResult.cpp
   core/cpp/StringArray.cpp
   core/cpp/Stream.cpp
   database/Database.cpp
   database/Collection.cpp
   database/CollectionIterator.cpp
   database/Resource.cpp
   database/Relationships.cpp
   database/Swagger.cpp
   database/Transaction.cpp
   restapi/jude_browser.c
   restapi/jude_rest_api.c
)

if (JUDE_USE_STDLIB)
   
    target_sources(jude
        PRIVATE porting/jude_port_std_c++11.cpp
    )
endif()

target_include_directories(jude PUBLIC
   ${PROJECT_SOURCE_DIR}/include
)
