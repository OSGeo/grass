#[[
File: FindGEOS.cmake

Find the native GEOS(Geometry Engine - Open Source) includes and libraries.

This module defines:

GEOS_INCLUDE_DIR, where to find geos.h, etc.

GEOS_LIBRARY, libraries to link against to use GEOS. Currently there are
two looked for, geos and geos_c libraries.

GEOS_FOUND, True if found, false if one of the above are not found.
For ossim, typically geos will be system installed which should be found;
or found in the ossim 3rd party dependencies directory from a geos build
and install. If the latter it will rely on CMAKE_INCLUDE_PATH and
CMAKE_LIBRARY_PATH having the path to the party dependencies directory.

NOTE:
This script is specialized for ossim, e.g. looking in /usr/local/ossim.

$Id$
---

---
Find include path:
Note: Ubuntu 14.04+ did not have geos.h (not included in any ossim src).
Instead looking for Geometry.h
---
#]]

find_path(GEOS_INCLUDE_DIR geos_c.h)

# Find GEOS C library:
find_library(GEOS_C_LIBRARY_RELEASE NAMES geos_c)
find_library(GEOS_C_LIBRARY_DEBUG NAMES geos_cd)
set(GEOS_FOUND FALSE)

set(GEOS_C_LIBRARY)
if(GEOS_C_LIBRARY_DEBUG)
  set(GEOS_C_LIBRARY ${GEOS_C_LIBRARY_DEBUG})
elseif(GEOS_C_LIBRARY_RELEASE)
  set(GEOS_C_LIBRARY ${GEOS_C_LIBRARY_RELEASE})
endif()

mark_as_advanced(GEOS_INCLUDE_DIR GEOS_C_LIBRARY GEOS_C_LIBRARY_RELEASE
                 GEOS_C_LIBRARY_DEBUG)

# --- This function sets GEOS_FOUND if variables are valid. ---
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GEOS DEFAULT_MSG GEOS_C_LIBRARY
                                  GEOS_INCLUDE_DIR)
