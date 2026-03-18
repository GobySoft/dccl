set(DCCL_CODEC_VERSION 4)
configure_file(test.proto.in ${CMAKE_CURRENT_SOURCE_DIR}/test4.proto)
configure_file(test.proto.in ${dccl_INC_DIR}/dccl/test/dccl_dynamic_conditions/test4.proto)
protobuf_generate_cpp_dccl(PROTO_SRCS4 PROTO_HDRS4 test4.proto)

set(DCCL_CODEC_VERSION 5)
configure_file(test.proto.in ${CMAKE_CURRENT_SOURCE_DIR}/test5.proto)
configure_file(test.proto.in ${dccl_INC_DIR}/dccl/test/dccl_dynamic_conditions/test5.proto)
protobuf_generate_cpp_dccl(PROTO_SRCS5 PROTO_HDRS5 test5.proto)

add_executable(dccl_test_dynamic_conditions_v4 test.cpp ${PROTO_SRCS4} ${PROTO_HDRS4})
target_link_libraries(dccl_test_dynamic_conditions_v4 dccl)

target_compile_definitions(dccl_test_dynamic_conditions_v4 PRIVATE -DCODEC_VERSION=4)

add_test(dccl_test_dynamic_conditions_v4 ${dccl_BIN_DIR}/dccl_test_dynamic_conditions_v4)

add_executable(dccl_test_dynamic_conditions_v5 test.cpp ${PROTO_SRCS5} ${PROTO_HDRS5})
target_link_libraries(dccl_test_dynamic_conditions_v5 dccl)

target_compile_definitions(dccl_test_dynamic_conditions_v5 PRIVATE -DCODEC_VERSION=5)

add_test(dccl_test_dynamic_conditions_v5 ${dccl_BIN_DIR}/dccl_test_dynamic_conditions_v5)
