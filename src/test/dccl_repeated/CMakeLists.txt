protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto ../dccl_header/header.proto)

add_executable(dccl_test_repeated test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(dccl_test_repeated dccl)

add_test(dccl_test_repeated ${dccl_BIN_DIR}/dccl_test_repeated)
