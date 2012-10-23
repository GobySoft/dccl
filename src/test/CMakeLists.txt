add_subdirectory(dccl_all_fields)
add_subdirectory(dccl_custom_message)
add_subdirectory(dccl_header)
add_subdirectory(dccl_repeated)
# add_subdirectory(dccl_hooks) tests hooks
add_subdirectory(dccl_default_id)
add_subdirectory(dccl_required_optional)
add_subdirectory(dccl_static_methods)

if(build_ccl)
  add_subdirectory(dccl_custom_id_ccl)
endif()

if(build_arithmetic)
  add_subdirectory(dccl_arithmetic)
endif()

add_subdirectory(logger1)
