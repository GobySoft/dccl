protobuf_generate_cpp_dccl(PROTO_SRCS PROTO_HDRS test.proto)

add_executable(dccl_test_issue149 test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(dccl_test_issue149 dccl)

add_test(dccl_test_issue149 ${dccl_BIN_DIR}/dccl_test_issue149)
