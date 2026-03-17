set(PROTOS
  option_extensions.proto
 )

# can't use the plugin before we generate it
protobuf_generate_cpp_no_dccl(PROTO_SRCS PROTO_HDRS ${PROTOS})

add_executable(protoc-gen-dccl pb_plugin.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(protoc-gen-dccl protobuf::libprotobuf protobuf::libprotoc ${CMAKE_THREAD_LIBS_INIT})
install(TARGETS protoc-gen-dccl DESTINATION ${CMAKE_INSTALL_BINDIR})

install(PROGRAMS protoc-gen-dccl_cpp DESTINATION ${CMAKE_INSTALL_BINDIR})
add_custom_command(OUTPUT ${dccl_BIN_DIR}/protoc-gen-dccl_cpp
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/protoc-gen-dccl_cpp ${dccl_BIN_DIR}/protoc-gen-dccl_cpp)
add_custom_target(protoc-gen-dccl_cpp_link ALL DEPENDS ${dccl_BIN_DIR}/protoc-gen-dccl_cpp)
