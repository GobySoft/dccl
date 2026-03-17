set(PROTOS
  option_extensions.proto
 )

# can't use the plugin before we generate it
protobuf_generate_cpp_no_dccl(PROTO_SRCS PROTO_HDRS ${PROTOS})

add_executable(protoc-gen-dccl pb_plugin.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(protoc-gen-dccl protobuf::libprotobuf protobuf::libprotoc ${CMAKE_THREAD_LIBS_INIT})
install(TARGETS protoc-gen-dccl DESTINATION ${CMAKE_INSTALL_BINDIR})


#add_executable(protoc-gen-dccl_cpp pb_plugin.cpp ${PROTO_SRCS} ${PROTO_HDRS})
#target_link_libraries(protoc-gen-dccl_cpp ${PROTOBUF_LIBRARIES} ${PROTOBUF_PROTOC_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
#install(TARGETS protoc-gen-dccl_cpp DESTINATION ${CMAKE_INSTALL_BINDIR})
#target_compile_definitions(protoc-gen-dccl_cpp PRIVATE INCLUDE_CPP)

# Install the bash script that replaces the compiled protoc-gen-dccl_cpp plugin.
# It translates --dccl_cpp_out=<dir> into --cpp_out=<dir> --dccl_out=<dir>
# and delegates to protoc + protoc-gen-dccl.
install(PROGRAMS protoc-gen-dccl_cpp DESTINATION ${CMAKE_INSTALL_BINDIR})
