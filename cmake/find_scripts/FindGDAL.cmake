find_path(GDAL_INCLUDE_DIR gdal.h)

find_library(GDAL_LIBRARY_RELEASE NAMES gdal_i gdal)
find_library(GDAL_LIBRARY_DEBUG NAMES gdald)
set(GDAL_FOUND FALSE)

set(GDAL_LIBRARY)
if(GDAL_LIBRARY_DEBUG)
  set( GDAL_LIBRARY ${GDAL_LIBRARY_DEBUG})
elseif(GDAL_LIBRARY_RELEASE)
  set( GDAL_LIBRARY ${GDAL_LIBRARY_RELEASE})
endif()

#---
# This function sets GEOS_FOUND if variables are valid.
#--- 
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args( GDAL DEFAULT_MSG 
                                   GDAL_LIBRARY
                                   GDAL_INCLUDE_DIR )

