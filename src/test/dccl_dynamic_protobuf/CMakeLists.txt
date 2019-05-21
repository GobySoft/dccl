# shared library suffix
add_definitions(-DSHARED_LIBRARY_SUFFIX="${CMAKE_SHARED_LIBRARY_SUFFIX}")

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test_a.proto)
add_executable(dccl_test_dynamic_protobuf dynamic_protobuf.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(dccl_test_dynamic_protobuf dccl ${Boost_LIBRARIES} ${PROTOBUF_LIBRARIES} ${CMAKE_DL_LIBS})
add_test(dccl_test_dynamic_protobuf ${dccl_BIN_DIR}/dccl_test_dynamic_protobuf)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test_b.proto)
add_library(test_dyn_protobuf SHARED ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(test_dyn_protobuf dl dccl ${PROTOBUF_LIBRARIES})
