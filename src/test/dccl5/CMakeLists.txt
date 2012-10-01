protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto)

add_executable(dccl_test5 test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(dccl_test5 dccl)

add_test(dccl_test5 ${dccl_BIN_DIR}/dccl_test5)
