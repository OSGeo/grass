###############################################################################
# CMake module to search for PROJ.4 library
#
# On success, the macro sets the following variables:
# PROJ4_FOUND       = if the library found
# PROJ4_LIBRARY     = full path to the library
# PROJ4_INCLUDE_DIR = where to find the library headers 
# also defined, but not for general use are
# PROJ4_LIBRARY, where to find the PROJ.4 library.
#
# Copyright (c) 2009 Mateusz Loskot <mateusz@loskot.net>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
###############################################################################

FIND_PATH(PROJ_INCLUDE_DIR proj_api.h
    DOC "Path to PROJ.4 include directory")
 
 file(READ "${PROJ_INCLUDE_DIR}/proj_api.h" _proj_api_h_CONTENTS)
  string(REGEX REPLACE ".*# *define PJ_VERSION.([0-9]+).*" "\\1" PROJ_VERSION_STRING "${_proj_api_h_CONTENTS}")
  #if(MUPARSER_VERSION MATCHES "^[0-9]+\$")
  #  set(MUPARSER_VERSION "${MUPARSER_VERSION}.0.0")
FIND_LIBRARY(PROJ_LIBRARY
    NAMES proj proj_i proj_d
    DOC "Path to PROJ library file")

if(PROJ_INCLUDE_DIR AND PROJ_LIBRARY)
set(PROJ_FOUND TRUE)
endif()
IF(PROJ_FOUND)
  SET(PROJ_LIBRARIES ${PROJ_LIBRARY})
ENDIF()
# Handle the QUIETLY and REQUIRED arguments and set SPATIALINDEX_FOUND to TRUE
# if all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PROJ 
	FOUND_VAR PROJ_FOUND
	REQUIRED_VARS  PROJ_LIBRARY PROJ_INCLUDE_DIR PROJ_VERSION_STRING
	VERSION_VAR PROJ_VERSION_STRING)

