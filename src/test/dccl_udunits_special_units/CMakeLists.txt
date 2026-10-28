add_executable(dccl_test_udunits_special_units test.cpp
  ${dccl_SRC_DIR}/units/udunits_special_units.cpp)
target_link_libraries(dccl_test_udunits_special_units UDUNITS2::UDUNITS2)

add_test(dccl_test_udunits_special_units ${dccl_BIN_DIR}/dccl_test_udunits_special_units)
