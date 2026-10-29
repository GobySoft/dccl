# Finds the UDUNITS-2 library (https://github.com/Unidata/UDUNITS-2)
#
# Defines:
#   UDUNITS2_FOUND
#   UDUNITS2_INCLUDE_DIRS
#   UDUNITS2_LIBRARIES
#   UDUNITS2::UDUNITS2 (imported target)

# Debian/Ubuntu install the header directly in the include directory, others in udunits2/
find_path(UDUNITS2_INCLUDE_DIR udunits2.h PATH_SUFFIXES udunits2)
find_library(UDUNITS2_LIBRARY NAMES udunits2)

mark_as_advanced(UDUNITS2_INCLUDE_DIR UDUNITS2_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UDUNITS2 DEFAULT_MSG UDUNITS2_LIBRARY UDUNITS2_INCLUDE_DIR)

if(UDUNITS2_FOUND)
  set(UDUNITS2_INCLUDE_DIRS ${UDUNITS2_INCLUDE_DIR})
  set(UDUNITS2_LIBRARIES ${UDUNITS2_LIBRARY})
  if(NOT TARGET UDUNITS2::UDUNITS2)
    add_library(UDUNITS2::UDUNITS2 UNKNOWN IMPORTED)
    set_target_properties(UDUNITS2::UDUNITS2 PROPERTIES
      IMPORTED_LOCATION "${UDUNITS2_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${UDUNITS2_INCLUDE_DIR}")
  endif()
endif()
