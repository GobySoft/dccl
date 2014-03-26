# so we can run this in the Debian package build environment
set(CMAKE_SKIP_RPATH OFF)

add_subdirectory(dccl_all_fields)
add_subdirectory(dccl_custom_message)
add_subdirectory(dccl_header)
add_subdirectory(dccl_repeated)
add_subdirectory(dccl_default_id)
add_subdirectory(dccl_required_optional)
add_subdirectory(dccl_static_methods)
add_subdirectory(dccl_custom_id)
add_subdirectory(dccl_numeric_bounds)
add_subdirectory(dccl_codec_group)

if(build_ccl)
  add_subdirectory(dccl_ccl)
endif()

if(build_arithmetic)
  add_subdirectory(dccl_arithmetic)
endif()

add_subdirectory(bitset1)

add_subdirectory(logger1)
add_subdirectory(round1)
