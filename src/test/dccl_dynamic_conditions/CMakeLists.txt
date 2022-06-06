set(DCCL_CODEC_VERSION 3)
configure_file(test.proto.in ${CMAKE_CURRENT_SOURCE_DIR}/test3.proto)
configure_file(test.proto.in ${dccl_INC_DIR}/dccl/test/dccl_dynamic_conditions/test3.proto)
protobuf_generate_cpp(PROTO_SRCS3 PROTO_HDRS3 test3.proto)

set(DCCL_CODEC_VERSION 4)
set(TEST_ONEOF
  "oneof example_oneof
    {
        int32 x = 20 [(dccl.field) = {
            min: 0
            max: 255
            dynamic_conditions { only_if: \"this.state == 'STATE_1'\" }
        }];
        int32 y = 21 [(dccl.field) = {
            min: 0
            max: 255
            dynamic_conditions { only_if: \"this.state == 'STATE_2'\" }
        }];
    }")
configure_file(test.proto.in ${CMAKE_CURRENT_SOURCE_DIR}/test4.proto)
configure_file(test.proto.in ${dccl_INC_DIR}/dccl/test/dccl_dynamic_conditions/test4.proto)
protobuf_generate_cpp(PROTO_SRCS4 PROTO_HDRS4 test4.proto)

add_executable(dccl_test_dynamic_conditions_v3 test.cpp ${PROTO_SRCS3} ${PROTO_HDRS3})
target_link_libraries(dccl_test_dynamic_conditions_v3 dccl)

target_compile_definitions(dccl_test_dynamic_conditions_v3 PRIVATE -DCODEC_VERSION=3)

add_test(dccl_test_dynamic_conditions_v3 ${dccl_BIN_DIR}/dccl_test_dynamic_conditions_v3)


add_executable(dccl_test_dynamic_conditions_v4 test.cpp ${PROTO_SRCS4} ${PROTO_HDRS4})
target_link_libraries(dccl_test_dynamic_conditions_v4 dccl)

target_compile_definitions(dccl_test_dynamic_conditions_v4 PRIVATE -DCODEC_VERSION=4)

add_test(dccl_test_dynamic_conditions_v4 ${dccl_BIN_DIR}/dccl_test_dynamic_conditions_v4)
