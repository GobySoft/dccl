protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test_arithmetic.proto)

add_executable(dccl_test_arithmetic test.cpp ${PROTO_SRCS} ${PROTO_HDRS})

target_compile_definitions(dccl_test_arithmetic PRIVATE DCCL_ARITHMETIC_NAME="$<TARGET_SONAME_FILE_NAME:dccl_arithmetic>")
target_link_libraries(dccl_test_arithmetic dccl dccl_arithmetic)

add_test(dccl_test_arithmetic ${dccl_BIN_DIR}/dccl_test_arithmetic)
