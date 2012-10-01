protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto ../dccl3/header.proto)

add_executable(dccl_test4 test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(dccl_test4 dccl)

add_test(dccl_test4 ${dccl_BIN_DIR}/dccl_test4)
