###############################################################################
# CMake module to search for PROJ.4 library
#
# On success, the macro sets the following variables:
# PROJ_FOUND       = if the library found
# PROJ_LIBRARY     = full path to the library
# PROJ_INCLUDE_DIR = where to find the library headers 
# also defined, but not for general use are
# PROJ_LIBRARY, where to find the PROJ.4 library.
#
# Copyright (c) 2009 Mateusz Loskot <mateusz@loskot.net>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
###############################################################################

FIND_PATH(PROJ_INCLUDE_DIR proj.h
    DOC "Path to PROJ.4 include directory 5.x +")

if(NOT PROJ_INCLUDE_DIR)
	FIND_PATH(PROJ_INCLUDE_DIR proj_api.h DOC "Path to PROJ.4 include directory 4.x")
	#file(READ "${PROJ_INCLUDE_DIR}/proj_api.h" _proj_h_CONTENTS)
endif()

if(EXISTS "${PROJ_INCLUDE_DIR}/proj.h")
	file(READ "${PROJ_INCLUDE_DIR}/proj.h" _proj_h_CONTENTS)
	 string(REGEX REPLACE ".*# *define PROJ_VERSION_MAJOR.([0-9])" "\\1" PROJ_VERSION_MAJOR "${_proj_h_CONTENTS}")
	 string(REGEX REPLACE ".*# *define PROJ_VERSION_MINOR.([0-9])" "\\1" PROJ_VERSION_MINOR "${_proj_h_CONTENTS}")
	 string(REGEX REPLACE ".*# *define PROJ_VERSION_PATCH.([0-9])" "\\1" PROJ_VERSION_PATCH "${_proj_h_CONTENTS}")

	 string(REGEX REPLACE ".*# *define PJ_VERSION.([0-9]+).*" "\\1" PROJ_VERSION_STRING "${_proj_h_CONTENTS}")
elseif(EXISTS "${PROJ_INCLUDE_DIR}/proj_api.h")
file(READ "${PROJ_INCLUDE_DIR}/proj_api.h" _proj_h_CONTENTS)
string(REGEX REPLACE ".*# *define PJ_VERSION.([0-9]+).*" "\\1" PROJ_VERSION_STRING "${_proj_h_CONTENTS}")
endif()

FIND_LIBRARY(PROJ_LIBRARY_RELEASE
    NAMES proj proj_i
    DOC "Path to PROJ library file")

FIND_LIBRARY(PROJ_LIBRARY_DEBUG
    NAMES projd
    DOC "Path to PROJ debug library file")

set(PROJ_LIBRARY)
if(PROJ_LIBRARY_DEBUG)
  set( PROJ_LIBRARY ${PROJ_LIBRARY_DEBUG})
elseif(PROJ_LIBRARY_RELEASE)
  set( PROJ_LIBRARY ${PROJ_LIBRARY_RELEASE})
endif()

if(PROJ_INCLUDE_DIR AND PROJ_LIBRARY)
set(PROJ_FOUND TRUE)
endif()


MARK_AS_ADVANCED(
  PROJ_INCLUDE_DIR
  PROJ_LIBRARY
  PROJ_LIBRARY_DEBUG
  PROJ_LIBRARY_RELEASE
)

# Handle the QUIETLY and REQUIRED arguments and set SPATIALINDEX_FOUND to TRUE
# if all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PROJ 
	FOUND_VAR PROJ_FOUND
	REQUIRED_VARS  PROJ_LIBRARY PROJ_INCLUDE_DIR PROJ_VERSION_STRING
	VERSION_VAR PROJ_VERSION_STRING)

