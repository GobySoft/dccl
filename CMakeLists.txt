## boost

if (enable_ncurses)
  message(WARNING "DCCL ignoring request to build with ncurses support!")
  remove_definitions(-DHAS_NCURSES)
endif(enable_ncurses)

goby_find_required_package(Boost 1.40.0 COMPONENTS filesystem)
include_directories(${Boost_INCLUDE_DIRS})

set(PROTOS ../common/protobuf/logger.proto
  ../common/protobuf/option_extensions.proto
  ../common/protobuf/acomms_option_extensions.proto
  protobuf/dccl.proto
  protobuf/ccl.proto)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})

add_library(dccl 
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
  ../common/time.cpp
  ../common/logger/logger_manipulators.cpp
  ../common/logger/term_color.cpp
  ../common/logger/flex_ostream.cpp
  ../common/logger/flex_ostreambuf.cpp

  ../util/dynamic_protobuf_manager.cpp
  
  ${PROTO_SRCS} ${PROTO_HDRS}
  )

target_link_libraries(dccl ${Cryptopp_LIBRARIES} ${PROTOBUF_LIBRARIES} ${CMAKE_DL_LIBS} ${Boost_LIBRARIES})
set_target_properties(dccl PROPERTIES VERSION "${GOBY_VERSION}" SOVERSION "${GOBY_SOVERSION}")

if(enable_testing)
  add_subdirectory(test)
endif()

if(build_apps)
  add_subdirectory(apps)
endif()
