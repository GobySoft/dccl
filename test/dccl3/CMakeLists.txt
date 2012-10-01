protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto header.proto)

add_executable(dccl_test3 test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(dccl_test3 dccl)

add_test(dccl_test3 ${dccl_BIN_DIR}/dccl_test3)
