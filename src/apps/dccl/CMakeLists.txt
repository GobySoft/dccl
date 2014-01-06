add_executable(dccl_tool dccl_tool.cpp)
target_link_libraries(dccl_tool dccl)
set_target_properties(dccl_tool PROPERTIES OUTPUT_NAME dccl)
