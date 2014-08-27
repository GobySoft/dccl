protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto header.proto)

add_executable(dccl_test_v2_header test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(dccl_test_v2_header dccl)

add_test(dccl_test_v2_header ${dccl_BIN_DIR}/dccl_test_v2_header)
