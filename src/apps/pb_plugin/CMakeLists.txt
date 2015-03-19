set(PROTOS
  option_extensions.proto
 )

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})

add_executable(protoc-gen-dccl pb_plugin.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(protoc-gen-dccl ${PROTOBUF_LIBRARIES} ${PROTOBUF_PROTOC_LIBRARY})
