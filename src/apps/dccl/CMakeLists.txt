protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS dccl_tool.proto)
add_executable(dccl_tool dccl_tool.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(dccl_tool dccl)
set_target_properties(dccl_tool PROPERTIES OUTPUT_NAME dccl)
