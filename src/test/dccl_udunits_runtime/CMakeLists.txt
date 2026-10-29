protobuf_generate_cpp_dccl(PROTO_SRCS PROTO_HDRS runtime.proto)

add_executable(dccl_test_udunits_runtime test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(dccl_test_udunits_runtime dccl ${CMAKE_THREAD_LIBS_INIT})

add_test(dccl_test_udunits_runtime ${dccl_BIN_DIR}/dccl_test_udunits_runtime)
