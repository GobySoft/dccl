# Drives protoc-gen-dccl as a subprocess, so there is nothing to compile here.
add_test(NAME dccl_test_pb_plugin
  COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test.sh
          ${PROTOBUF_PROTOC_EXECUTABLE}
          $<TARGET_FILE:protoc-gen-dccl>
          ${CMAKE_CURRENT_SOURCE_DIR}
          ${dccl_INC_DIR}
          ${CMAKE_CURRENT_BINARY_DIR}/generated)
