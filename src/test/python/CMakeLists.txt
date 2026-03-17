find_package(Python3 COMPONENTS Interpreter QUIET)

if(NOT Python3_FOUND)
    message(STATUS "Python3 not found; skipping Python DCCL tests.")
    return()
endif()

if(NOT TARGET dccl_python)
    message(STATUS "dccl_python build target not available; skipping Python DCCL tests.")
    return()
endif()

# Generate test_pb2.py using the CMake-discovered protoc.
# option_extensions_pb2.py is generated into ${dccl_PYTHON_DIR}/dccl/ by the
# dccl_python target, so only the test-specific proto needs to be generated here.
set(_pb2_output ${CMAKE_CURRENT_BINARY_DIR}/test_pb2.py)
add_custom_command(
    OUTPUT ${_pb2_output}
    COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
            --python_out=${CMAKE_CURRENT_BINARY_DIR}
            --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
            --proto_path=${dccl_INC_DIR}
            test.proto
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test.proto
    COMMENT "Generating Python protobuf for dccl Python tests"
)
add_custom_target(dccl_python_test_pb2 ALL DEPENDS ${_pb2_output})
# Ensure option_extensions_pb2.py exists before test_pb2.py is imported.
add_dependencies(dccl_python_test_pb2 dccl_python)

# Build the PYTHONPATH so the test can find:
#   - the dccl package (with _dccl.<soabi>.so and option_extensions_pb2.py)
#     from the in-place build in ${dccl_PYTHON_DIR}
#   - the generated test_pb2.py from ${CMAKE_CURRENT_BINARY_DIR}
set(_python_test_env
    "DCCL_TEST_PROTO_PATH=${CMAKE_CURRENT_SOURCE_DIR}"
    "DCCL_INC_PATH=${dccl_INC_DIR}"
    "PYTHONPATH=${dccl_PYTHON_DIR}:${CMAKE_CURRENT_BINARY_DIR}"
)
if(APPLE)
    list(APPEND _python_test_env "DYLD_LIBRARY_PATH=${dccl_LIB_DIR}")
else()
    list(APPEND _python_test_env "LD_LIBRARY_PATH=${dccl_LIB_DIR}")
endif()

add_test(
    NAME python_dccl_test
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_dccl.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_tests_properties(python_dccl_test PROPERTIES
    ENVIRONMENT "${_python_test_env}"
)
