protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto)

add_executable(dccl_test_native_protobuf test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_compile_definitions(dccl_test_native_protobuf PRIVATE DCCL_NATIVE_PROTOBUF_NAME="$<TARGET_SONAME_FILE_NAME:dccl_native_protobuf>")

target_link_libraries(dccl_test_native_protobuf dccl dccl_native_protobuf)

add_test(dccl_test_native_protobuf ${dccl_BIN_DIR}/dccl_test_native_protobuf)
