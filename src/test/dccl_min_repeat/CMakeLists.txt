protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto)
add_executable(dccl_test_min_repeat test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(dccl_test_min_repeat dccl)
add_test(dccl_test_min_repeat ${dccl_BIN_DIR}/dccl_test_min_repeat)
