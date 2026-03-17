find_package(Python3 COMPONENTS Interpreter)

if(NOT Python3_FOUND)
    message(STATUS "Python3 not found; skipping Python DCCL tests.")
    return()
endif()

# Check whether the dccl Python module is importable.
execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "import dccl"
    RESULT_VARIABLE _dccl_python_importable
    OUTPUT_QUIET
    ERROR_QUIET
)

if(NOT _dccl_python_importable EQUAL 0)
    message(STATUS "dccl Python module not importable; skipping Python DCCL tests.")
    return()
endif()

# Generate the Python pb2 file for the test proto.
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

# Also generate the option_extensions pb2 so protobuf can resolve imports.
set(_opt_pb2_output ${CMAKE_CURRENT_BINARY_DIR}/dccl/option_extensions_pb2.py)
add_custom_command(
    OUTPUT ${_opt_pb2_output}
    COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
            --python_out=${CMAKE_CURRENT_BINARY_DIR}
            --proto_path=${dccl_INC_DIR}
            dccl/option_extensions.proto
    COMMENT "Generating option_extensions_pb2 for dccl Python tests"
)
add_custom_target(dccl_python_option_ext_pb2 ALL DEPENDS ${_opt_pb2_output})

add_dependencies(dccl_python_test_pb2 dccl_python_option_ext_pb2)

add_test(
    NAME python_dccl_test
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_dccl.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_tests_properties(python_dccl_test PROPERTIES
    ENVIRONMENT "DCCL_TEST_PROTO_PATH=${CMAKE_CURRENT_SOURCE_DIR};DCCL_INC_PATH=${dccl_INC_DIR}"
)
