add_executable(protoc-gen-dccl pb_plugin.cpp)
target_link_libraries(protoc-gen-dccl ${PROTOBUF_LIBRARIES} ${PROTOBUF_PROTOC_LIBRARY})
