add_executable(dccl_test_unbiased_round1 test.cpp)
target_link_libraries(dccl_test_unbiased_round1 dccl)

add_test(dccl_test_unbiased_round1 ${dccl_BIN_DIR}/dccl_test_unbiased_round1)

