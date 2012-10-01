protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto ../dccl3/header.proto)

add_executable(dccl_test8 test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(dccl_test8 dccl)

add_test(dccl_test8 ${dccl_BIN_DIR}/dccl_test8)
