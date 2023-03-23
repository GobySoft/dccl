protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ../dccl_all_fields/test.proto ../dccl_arithmetic/test_arithmetic.proto)

add_executable(dccl_test_multithread test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(dccl_test_multithread dccl dccl_arithmetic)
target_compile_definitions(dccl_test_multithread PRIVATE DCCL_ARITHMETIC_NAME="$<TARGET_SONAME_FILE_NAME:dccl_arithmetic>")

add_test(dccl_test_multithread ${dccl_BIN_DIR}/dccl_test_multithread)
