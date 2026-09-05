# The tool is exercised as a subprocess, so there is nothing to compile here.
add_test(NAME dccl_test_tool
  COMMAND ${CMAKE_COMMAND} -E env
          bash ${CMAKE_CURRENT_SOURCE_DIR}/test.sh
          $<TARGET_FILE:dccl_tool>
          ${CMAKE_CURRENT_SOURCE_DIR}
          ${dccl_INC_DIR})
