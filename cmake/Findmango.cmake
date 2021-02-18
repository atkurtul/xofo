find_path(mango_INCLUDE_DIRS
  NAMES mango/mango.hpp
  HINTS include)

find_library(mango_LIBRARIES
  NAMES mango
  HINTS lib)


include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(mango
  DEFAULT_MSG
  mango_LIBRARIES mango_INCLUDE_DIRS)


mark_as_advanced(mango_INCLUDE_DIRS mango_LIBRARIES)

if(mango_FOUND AND NOT TARGET mango::mango)
  add_library(mango::mango UNKNOWN IMPORTED)
  set_target_properties(mango::mango PROPERTIES
    IMPORTED_LOCATION "${mango_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${mango_INCLUDE_DIRS}")
endif()
