find_path(GDAL_INCLUDE_DIR gdal.h PATH_SUFFIXES gdal)

find_library(GDAL_LIBRARY_RELEASE NAMES gdal_i gdal)
find_library(GDAL_LIBRARY_DEBUG NAMES gdald)
set(GDAL_FOUND FALSE)

set(GDAL_LIBRARY)
if(GDAL_LIBRARY_DEBUG)
  set(GDAL_LIBRARY
      ${GDAL_LIBRARY_DEBUG}
      CACHE FILEPATH "doc")
elseif(GDAL_LIBRARY_RELEASE)
  set(GDAL_LIBRARY
      ${GDAL_LIBRARY_RELEASE}
      CACHE FILEPATH "doc")
endif()

mark_as_advanced(GDAL_LIBRARY_RELEASE)
mark_as_advanced(GDAL_LIBRARY_DEBUG)
mark_as_advanced(GDAL_LIBRARY)
mark_as_advanced(GDAL_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GDAL DEFAULT_MSG GDAL_LIBRARY
                                  GDAL_INCLUDE_DIR)
