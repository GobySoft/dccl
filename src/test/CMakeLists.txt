# so we can run this in the Debian package build environment
set(CMAKE_SKIP_RPATH OFF)

add_subdirectory(dccl_all_fields)
add_subdirectory(dccl_custom_message)
add_subdirectory(dccl_header)
add_subdirectory(dccl_repeated)
add_subdirectory(dccl_default_id)
add_subdirectory(dccl_required_optional)
add_subdirectory(dccl_var_bytes)
add_subdirectory(dccl_static_methods)
add_subdirectory(dccl_custom_id)
add_subdirectory(dccl_user_specified_id)
add_subdirectory(dccl_numeric_bounds)
add_subdirectory(dccl_codec_group)
add_subdirectory(dccl_message_fix)
add_subdirectory(dccl_oneof)
add_subdirectory(dccl_strict)
add_subdirectory(dccl_packed_enum)
add_subdirectory(dccl_dynamic_protobuf)
add_subdirectory(dccl_presence)
add_subdirectory(dccl_resolution)
add_subdirectory(dccl_min_repeat)
add_subdirectory(dccl_hash)

  
if(enable_units)
  add_subdirectory(dccl_units)

  # requires DCCL protoc plugin, which requires units
  add_subdirectory(dccl_load_file)
endif()

if(build_ccl)
  add_subdirectory(dccl_ccl)
endif()

if(build_arithmetic)
  add_subdirectory(dccl_arithmetic)
  if(enable_thread_safety)
    add_subdirectory(dccl_multithread)
  endif()
endif()

if(build_native_protobuf)
  add_subdirectory(dccl_native_protobuf) 
endif()

add_subdirectory(dccl_v3_all_fields)
add_subdirectory(dccl_v2_all_fields)
add_subdirectory(dccl_v2_header)

add_subdirectory(bitset1)

add_subdirectory(logger1)
add_subdirectory(round1)

if(enable_lua)
  add_subdirectory(dccl_dynamic_conditions)
endif()
