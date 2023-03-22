protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ../dccl_all_fields/test.proto)

add_executable(dccl_test_multithread test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(dccl_test_multithread dccl)

add_test(dccl_test_multithread ${dccl_BIN_DIR}/dccl_test_multithread)
