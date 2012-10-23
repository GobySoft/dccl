protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto)

add_executable(dccl_test_arithmetic test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(dccl_test_arithmetic dccl dccl_arithmetic)

add_test(dccl_test_arithmetic ${dccl_BIN_DIR}/dccl_test_arithmetic)
