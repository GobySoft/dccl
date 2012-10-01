protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto)

add_executable(dccl_test10 test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(dccl_test10 dccl)

add_test(dccl_test10 ${dccl_BIN_DIR}/dccl_test10)
