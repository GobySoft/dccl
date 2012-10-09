
set(PROTOS protobuf/logger.proto
  protobuf/option_extensions.proto
  protobuf/dccl.proto
  protobuf/ccl.proto)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})

add_library(dccl 
  logger.cpp
  dccl.cpp
  dccl_field_codec.cpp
  dccl_field_codec_helpers.cpp
  dccl_field_codec_default.cpp
  dccl_field_codec_default_message.cpp
  dccl_field_codec_manager.cpp
  dccl_field_codec_arithmetic.cpp
  dccl_type_helper.cpp
  dccl_bitset.cpp
  dccl_ccl_compatibility.cpp
  WhoiUtil.cpp
  # COMMON
  junk/common/time.cpp
  junk/common/logger/logger_manipulators.cpp
  junk/common/logger/term_color.cpp
  junk/common/logger/flex_ostream.cpp
  junk/common/logger/flex_ostreambuf.cpp

  junk/util/dynamic_protobuf_manager.cpp
  
  ${PROTO_SRCS} ${PROTO_HDRS}
  )

target_link_libraries(dccl ${Cryptopp_LIBRARIES} ${PROTOBUF_LIBRARIES} ${CMAKE_DL_LIBS} ${Boost_LIBRARIES})
set_target_properties(dccl PROPERTIES VERSION "${DCCL_VERSION}" SOVERSION "${DCCL_SOVERSION}")

if(enable_testing)
  add_subdirectory(test)
endif()

if(build_apps)
  add_subdirectory(apps)
endif()
