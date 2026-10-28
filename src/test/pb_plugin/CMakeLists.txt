# Drives protoc-gen-dccl as a subprocess and checks the code it generates.
add_test(NAME dccl_test_pb_plugin
  COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test.sh
          ${PROTOBUF_PROTOC_EXECUTABLE}
          $<TARGET_FILE:protoc-gen-dccl>
          ${CMAKE_CURRENT_SOURCE_DIR}
          ${dccl_INC_DIR}
          ${CMAKE_CURRENT_BINARY_DIR}/generated
          $<IF:$<BOOL:${enable_units}>,ON,OFF>
          $<IF:$<BOOL:${enable_udunits}>,ON,OFF>)

# the generated units accessors must also compile and convert correctly
if(enable_units)
  protobuf_generate_cpp_dccl(PROTO_SRCS PROTO_HDRS units_syntax.proto)
  add_executable(dccl_test_units_syntax units_syntax_test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
  target_link_libraries(dccl_test_units_syntax dccl)
  add_test(dccl_test_units_syntax ${dccl_BIN_DIR}/dccl_test_units_syntax)
endif()

if(enable_udunits)
  protobuf_generate_cpp_dccl(UDUNITS_PROTO_SRCS UDUNITS_PROTO_HDRS udunits.proto)
  add_executable(dccl_test_udunits udunits_test.cpp ${UDUNITS_PROTO_SRCS} ${UDUNITS_PROTO_HDRS})
  target_link_libraries(dccl_test_udunits dccl)
  add_test(dccl_test_udunits ${dccl_BIN_DIR}/dccl_test_udunits)
endif()

# (dccl.field).udunits translated to Boost Units accessors
if(enable_units AND enable_udunits)
  protobuf_generate_cpp_dccl(UDUNITS_BOOST_PROTO_SRCS UDUNITS_BOOST_PROTO_HDRS udunits_boost.proto)
  add_executable(dccl_test_udunits_boost udunits_boost_test.cpp ${UDUNITS_BOOST_PROTO_SRCS}
    ${UDUNITS_BOOST_PROTO_HDRS})
  target_link_libraries(dccl_test_udunits_boost dccl)
  add_test(dccl_test_udunits_boost ${dccl_BIN_DIR}/dccl_test_udunits_boost)
endif()
