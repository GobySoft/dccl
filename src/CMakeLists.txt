set(PROTOS
  option_extensions.proto
  protobuf/option_extensions.proto
 )

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})

set(SRC
  logger.cpp
  codec.cpp
  field_codec.cpp
  field_codec_manager.cpp
  field_codec_id.cpp
  bitset.cpp
  dynamic_protobuf_manager.cpp
  dynamic_conditions.cpp
  codecs2/field_codec_default.cpp
  codecs2/field_codec_default_message.cpp
  codecs3/field_codec_default_message.cpp
  codecs3/field_codec_default.cpp
  codecs3/field_codec_var_bytes.cpp
  codecs4/field_codec_default_message.cpp
  internal/type_helper.cpp
  internal/field_codec_message_stack.cpp
  thread_safety.cpp
  ${PROTO_SRCS} ${PROTO_HDRS}
  ) 


# embed lua-protobuf directly into libdccl.so to avoid require path headaches
if(enable_lua)
  set(SRC
    ${SRC}
    thirdparty/lua-protobuf/pb.c
    )
endif()

add_library(dccl 
  ${SRC}
  )

target_link_libraries(dccl ${PROTOBUF_LIBRARIES} ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT})

if(enable_b64)
  target_link_libraries(dccl ${B64_LIBRARIES})
endif()

if(enable_units)
  add_dependencies(dccl protoc-gen-dccl)
endif()

if(enable_cryptography)
  target_link_libraries(dccl ${Cryptopp_LIBRARIES})
endif()

if(enable_lua)
  target_link_libraries(dccl ${LUA_LIBRARY})
endif()

set_target_properties(dccl PROPERTIES VERSION "${DCCL_VERSION}" SOVERSION "${DCCL_SOVERSION}")

set_property(TARGET dccl APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
  $<BUILD_INTERFACE:${dccl_INC_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>  # <prefix>/include
)

install(TARGETS dccl EXPORT dccl-config
   LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
   ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}) 

set(DCCL_EXPORT_TARGETS dccl)

option(build_arithmetic "Build arithmetic coders shared library" ON)
option(build_ccl "Build Compact Control Language (CCL) legacy support shared library" ON)
option(build_native_protobuf "Build Native Protobuf encoded fields shared library" ON)
option(build_doc "Build documentation (requires Doxygen [and LaTeX for PDF generation])" OFF)

if(build_doc)
  add_subdirectory(doc)
endif()

if(build_ccl)
  if(NOT BUILD_SHARED_LIBS)
    message(WARNING "Cannot build dccl_ccl_compat as a static library")
    set(build_ccl OFF)
  else()
    add_subdirectory(ccl)
    set(DCCL_EXPORT_TARGETS ${DCCL_EXPORT_TARGETS} dccl_ccl_compat)
  endif()
endif()

if(build_arithmetic)
  if(NOT BUILD_SHARED_LIBS)
    message(WARNING "Cannot build dccl_arithmetic as a static library")
    set(build_arithmetic OFF)
  else()
    add_subdirectory(arithmetic)
    set(DCCL_EXPORT_TARGETS ${DCCL_EXPORT_TARGETS} dccl_arithmetic)
  endif()
endif()

if(build_native_protobuf)
  if(NOT BUILD_SHARED_LIBS)
    message(WARNING "Cannot build dccl_native_protobuf as a static library")
    set(build_native_protobuf OFF)
  else()
    add_subdirectory(native_protobuf)
    set(DCCL_EXPORT_TARGETS ${DCCL_EXPORT_TARGETS} dccl_native_protobuf)
  endif()
endif()

export(TARGETS ${DCCL_EXPORT_TARGETS} FILE ${CMAKE_BINARY_DIR}/dccl-config.cmake)

if(enable_testing)
  add_subdirectory(test)
endif()

if(build_apps)
  add_subdirectory(apps)
endif()
