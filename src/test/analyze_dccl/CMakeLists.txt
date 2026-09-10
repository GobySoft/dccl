# Runs the app as a subprocess; reuses the dccl tool test's .proto.
# protoc rejects ".." inside a virtual path, so normalize it here.
get_filename_component(_analyze_proto_dir "${CMAKE_CURRENT_SOURCE_DIR}/../dccl_tool" ABSOLUTE)

add_test(NAME dccl_test_analyze_dccl
  COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test.sh
          $<TARGET_FILE:analyze_dccl>
          ${_analyze_proto_dir}
          ${dccl_INC_DIR})
