add_library(dccl_native_protobuf SHARED
  dccl_native_protobuf.cpp
)

target_link_libraries(dccl_native_protobuf dccl)

set_target_properties(dccl_native_protobuf
  PROPERTIES VERSION "${DCCL_VERSION}" SOVERSION "${DCCL_SOVERSION}")

install(TARGETS dccl_native_protobuf EXPORT dccl-config
   LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
   ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}) 

 
