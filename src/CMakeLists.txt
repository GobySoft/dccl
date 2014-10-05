
set(PROTOS
  protobuf/option_extensions.proto
 )

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})

add_library(dccl 
  logger.cpp
  codec.cpp
  field_codec.cpp
  field_codec_manager.cpp
  field_codec_id.cpp
  bitset.cpp
  dynamic_protobuf_manager.cpp
  codecs2/field_codec_default.cpp
  codecs2/field_codec_default_message.cpp
  codecs3/field_codec_default_message.cpp
  codecs3/field_codec_default.cpp
  b64/cencode.c
  b64/cdecode.c
  internal/type_helper.cpp
  internal/field_codec_message_stack.cpp
  ${PROTO_SRCS} ${PROTO_HDRS}
  )

target_link_libraries(dccl ${PROTOBUF_LIBRARIES} ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT})

if(enable_cryptography)
  target_link_libraries(dccl ${Cryptopp_LIBRARIES})
endif()


set_target_properties(dccl PROPERTIES VERSION "${DCCL_VERSION}" SOVERSION "${DCCL_SOVERSION}")


install(TARGETS dccl EXPORT dccl-config
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib) 

set(DCCL_EXPORT_TARGETS dccl)

option(build_arithmetic "Build arithmetic coders shared library" ON)
option(build_ccl "Build Compact Control Language (CCL) legacy support shared library" ON)
option(build_doc "Build documentation (requires Doxygen [and LaTeX for PDF generation])" OFF)

if(build_doc)
  add_subdirectory(doc)
endif()

if(build_ccl)
  add_subdirectory(ccl)
  set(DCCL_EXPORT_TARGETS ${DCCL_EXPORT_TARGETS} dccl_ccl_compat)
endif()

if(build_arithmetic)
  add_subdirectory(arithmetic)
  set(DCCL_EXPORT_TARGETS ${DCCL_EXPORT_TARGETS} dccl_arithmetic)
endif()

export(TARGETS ${DCCL_EXPORT_TARGETS} FILE ${CMAKE_BINARY_DIR}/dccl-config.cmake)
file(APPEND ${CMAKE_BINARY_DIR}/dccl-config.cmake "set(DCCL_INCLUDE_DIR \"${dccl_INC_DIR}\")")

if(enable_testing)
  add_subdirectory(test)
endif()

if(build_apps)
  add_subdirectory(apps)
endif()
