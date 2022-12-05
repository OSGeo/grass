find_path(PROJ_INCLUDE_DIR proj.h PATH_SUFFIXES proj)

find_library(PROJ_LIBRARY_RELEASE NAMES proj_i proj)
find_library(PROJ_LIBRARY_DEBUG NAMES projd)
set(PROJ_FOUND FALSE)

set(PROJ_LIBRARY)
if(PROJ_LIBRARY_DEBUG)
  set( PROJ_LIBRARY ${PROJ_LIBRARY_DEBUG} CACHE FILEPATH "doc" )
elseif(PROJ_LIBRARY_RELEASE)
  set( PROJ_LIBRARY ${PROJ_LIBRARY_RELEASE} CACHE FILEPATH "doc" )
endif()

mark_as_advanced(PROJ_LIBRARY_RELEASE)
mark_as_advanced(PROJ_LIBRARY_DEBUG)
mark_as_advanced(PROJ_LIBRARY)
mark_as_advanced(PROJ_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args( PROJ DEFAULT_MSG
                                   PROJ_LIBRARY
                                   PROJ_INCLUDE_DIR )

