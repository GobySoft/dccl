
set(CCL_PROTOS 
  protobuf/ccl.proto
  protobuf/ccl_extensions.proto)

protobuf_generate_cpp(CCL_PROTO_SRCS CCL_PROTO_HDRS ${CCL_PROTOS})

add_library(dccl_ccl_compat SHARED
  ccl_compatibility.cpp
  WhoiUtil.cpp
  ${CCL_PROTO_SRCS}
  ${CCL_PROTO_HDRS}
)

target_link_libraries(dccl_ccl_compat dccl)

set_target_properties(dccl_ccl_compat 
  PROPERTIES VERSION "${DCCL_VERSION}" SOVERSION "${DCCL_SOVERSION}")
