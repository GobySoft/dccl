
set(PROTOS
  protobuf/option_extensions.proto
 )

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})

add_library(dccl 
  logger.cpp
  codec.cpp
  field_codec.cpp
  field_codec_message_stack.cpp
  field_codec_default.cpp
  field_codec_default_message.cpp
  field_codec_manager.cpp
  type_helper.cpp
  bitset.cpp
  dynamic_protobuf_manager.cpp
  
  ${PROTO_SRCS} ${PROTO_HDRS}
  )

target_link_libraries(dccl ${Cryptopp_LIBRARIES} ${PROTOBUF_LIBRARIES} ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT})
set_target_properties(dccl PROPERTIES VERSION "${DCCL_VERSION}" SOVERSION "${DCCL_SOVERSION}")


option(build_arithmetic "Build arithmetic coders shared library" ON)
option(build_ccl "Build Compact Control Language (CCL) legacy support shared library" ON)

if(build_ccl)
  add_subdirectory(ccl)
endif()

if(build_arithmetic)
  add_subdirectory(arithmetic)
endif()

if(enable_testing)
  add_subdirectory(test)
endif()

if(build_apps)
  add_subdirectory(apps)
endif()
