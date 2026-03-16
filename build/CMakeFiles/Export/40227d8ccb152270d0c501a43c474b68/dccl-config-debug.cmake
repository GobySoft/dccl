#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "dccl" for configuration "Debug"
set_property(TARGET dccl APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(dccl PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libdccl.so.4.3.1+2+gitc8bb1e8b-dirty"
  IMPORTED_SONAME_DEBUG "libdccl.so.31"
  )

list(APPEND _cmake_import_check_targets dccl )
list(APPEND _cmake_import_check_files_for_dccl "${_IMPORT_PREFIX}/lib/libdccl.so.4.3.1+2+gitc8bb1e8b-dirty" )

# Import target "dccl_ccl_compat" for configuration "Debug"
set_property(TARGET dccl_ccl_compat APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(dccl_ccl_compat PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libdccl_ccl_compat.so.4.3.1+2+gitc8bb1e8b-dirty"
  IMPORTED_SONAME_DEBUG "libdccl_ccl_compat.so.31"
  )

list(APPEND _cmake_import_check_targets dccl_ccl_compat )
list(APPEND _cmake_import_check_files_for_dccl_ccl_compat "${_IMPORT_PREFIX}/lib/libdccl_ccl_compat.so.4.3.1+2+gitc8bb1e8b-dirty" )

# Import target "dccl_arithmetic" for configuration "Debug"
set_property(TARGET dccl_arithmetic APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(dccl_arithmetic PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libdccl_arithmetic.so.4.3.1+2+gitc8bb1e8b-dirty"
  IMPORTED_SONAME_DEBUG "libdccl_arithmetic.so.31"
  )

list(APPEND _cmake_import_check_targets dccl_arithmetic )
list(APPEND _cmake_import_check_files_for_dccl_arithmetic "${_IMPORT_PREFIX}/lib/libdccl_arithmetic.so.4.3.1+2+gitc8bb1e8b-dirty" )

# Import target "dccl_native_protobuf" for configuration "Debug"
set_property(TARGET dccl_native_protobuf APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(dccl_native_protobuf PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libdccl_native_protobuf.so.4.3.1+2+gitc8bb1e8b-dirty"
  IMPORTED_SONAME_DEBUG "libdccl_native_protobuf.so.31"
  )

list(APPEND _cmake_import_check_targets dccl_native_protobuf )
list(APPEND _cmake_import_check_files_for_dccl_native_protobuf "${_IMPORT_PREFIX}/lib/libdccl_native_protobuf.so.4.3.1+2+gitc8bb1e8b-dirty" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
