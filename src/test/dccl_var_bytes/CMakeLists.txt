protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto)

add_executable(dccl_test_var_bytes test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(dccl_test_var_bytes dccl)

add_test(dccl_test_var_bytes ${dccl_BIN_DIR}/dccl_test_var_bytes)
