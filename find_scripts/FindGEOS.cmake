#---
# File: FindGEOS.cmake
#
# Find the native GEOS(Geometry Engine - Open Source) includes and libraries.
#
# This module defines:
#
# GEOS_INCLUDE_DIR, where to find geos.h, etc.
# GEOS_LIBRARY, libraries to link against to use GEOS.  Currently there are
# two looked for, geos and geos_c libraries.
# GEOS_FOUND, True if found, false if one of the above are not found.
# 
# For ossim, typically geos will be system installed which should be found; 
# or found in the ossim 3rd party dependencies directory from a geos build 
# and install.  If the latter it will rely on CMAKE_INCLUDE_PATH and 
# CMAKE_LIBRARY_PATH having the path to the party dependencies directory.
# 
# NOTE: 
# This script is specialized for ossim, e.g. looking in /usr/local/ossim.
#
# $Id$
#---

#---
# Find include path:
# Note: Ubuntu 14.04+ did not have geos.h (not included in any ossim src). 
# Instead looking for Geometry.h
#---

find_path( GEOS_INCLUDE_DIR geos/geom/Geometry.h
           PATHS 
           ${CMAKE_INSTALL_PREFIX}/include
           $ENV{GEOS_DIR}/include
           ${GEOS_DIR}/include
           /usr/include
           /usr/local/include
           /usr/local/ossim/include )

# Find GEOS C library:
find_library( GEOS_C_LIB NAMES geos_c )

set(GEOS_FOUND FALSE)

set( GEOS_LIBRARIES)
if(GEOS_C_LIB)
  set( GEOS_LIBRARIES ${GEOS_C_LIB})
endif()
# # Find GEOS c++ library:
# find_library( GEOS_CPP_LIB NAMES geos )

if(GEOS_CPP_LIB)
  set(GEOS_LIBRARIES ${GEOS_C_LIB} ${GEOS_CPP_LIB} )
endif()

#---
# This function sets GEOS_FOUND if variables are valid.
#--- 
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args( GEOS DEFAULT_MSG 
                                   GEOS_LIBRARIES
                                   GEOS_INCLUDE_DIR )

