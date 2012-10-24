set(ARITHMETIC_PROTOS 
protobuf/arithmetic_extensions.proto
protobuf/arithmetic.proto
)

protobuf_generate_cpp(ARITHMETIC_PROTO_SRCS
  ARITHMETIC_PROTO_HDRS
  ${ARITHMETIC_PROTOS})


add_library(dccl_arithmetic SHARED
  field_codec_arithmetic.cpp
  ${ARITHMETIC_PROTO_SRCS}
  ${ARITHMETIC_PROTO_HDRS}
)

set_target_properties(dccl_arithmetic 
  PROPERTIES VERSION "${DCCL_VERSION}" SOVERSION "${DCCL_SOVERSION}")
