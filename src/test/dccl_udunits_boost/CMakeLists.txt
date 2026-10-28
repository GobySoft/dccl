protobuf_generate_cpp_dccl(PROTO_SRCS PROTO_HDRS auv_status.proto)

add_executable(dccl_test_udunits_boost_codec test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(dccl_test_udunits_boost_codec dccl)

add_test(dccl_test_udunits_boost_codec ${dccl_BIN_DIR}/dccl_test_udunits_boost_codec)
