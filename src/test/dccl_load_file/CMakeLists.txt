# shared library suffix
add_definitions(-DSHARED_LIBRARY_SUFFIX="${CMAKE_SHARED_LIBRARY_SUFFIX}")

set(load_file ${CMAKE_CURRENT_BINARY_DIR}/dccl_load.cpp)
protobuf_generate_cpp_load_file(${load_file} PROTO_SRCS PROTO_HDRS test1.proto test2.proto)

add_library(test_autoload SHARED ${load_file} ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(test_autoload dccl)

add_executable(dccl_test_load_file test.cpp)
target_link_libraries(dccl_test_load_file dccl test_autoload)

add_test(dccl_test_load_file ${dccl_BIN_DIR}/dccl_test_load_file)
