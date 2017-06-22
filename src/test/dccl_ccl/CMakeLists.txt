protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto)

add_executable(dccl_test_ccl test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_compile_definitions(dccl_test_ccl PRIVATE DCCL_CCL_COMPAT_NAME="$<TARGET_SONAME_FILE_NAME:dccl_ccl_compat>")

target_link_libraries(dccl_test_ccl dccl dccl_ccl_compat)

add_test(dccl_test_ccl ${dccl_BIN_DIR}/dccl_test_ccl)

add_custom_target(
  GenexDemo
  COMMAND ${CMAKE_COMMAND} -E echo "$<TARGET_FILE:dccl_ccl_compat>"
  VERBATIM
)
