set(PROTOS
  option_extensions.proto
 )

# can't use the plugin before we generate it
protobuf_generate_cpp_no_dccl(PROTO_SRCS PROTO_HDRS ${PROTOS})

add_executable(protoc-gen-dccl pb_plugin.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(protoc-gen-dccl ${PROTOBUF_LIBRARIES} ${PROTOBUF_PROTOC_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
install(TARGETS protoc-gen-dccl DESTINATION ${CMAKE_INSTALL_BINDIR})
