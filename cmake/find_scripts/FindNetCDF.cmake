#[[
COPYRIGHT: (c) 2020-2025 Rashad Kanavath and the GRASS Development Team
SPDX-License-Identifier: GPL-2.0-or-later
]]

#[=======================================================================[.rst:
FindNetCDF
----------

Find the Network Common Data Form (NetCDF) library

.. _`NetCDF`: https://www.unidata.ucar.edu/software/netcdf/

Imported targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` targets:

``NetCDF::NetCDF``

  The libraries to use for NetCDF, if found.

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables:

``NetCDF_FOUND``
  library implementing the NetCDF interface is found
``NetCDF_LIBRARY``
  library to link against to use NetCDF
``NetCDF_INCLUDEDIR``
  path to the NetCDF include directory.
``NetCDF_VERSION``
  version of the NetCDF library

#]=======================================================================]

find_package(PkgConfig QUIET)
pkg_check_modules(PC_NetCDF netcdf QUIET)

find_path(
  NetCDF_INCLUDEDIR
  NAMES netcdf.h
  HINTS ${PC_NetCDF_INCLUDEDIR} ${PC_NetCDF_INCLUDE_DIRS}
  DOC "path to netcdf.h")

find_library(
  NetCDF_LIBRARY
  NAMES netcdf
  HINTS ${PC_NetCDF_LIBDIR} ${PC_NetCDF_LIBRARY_DIRS}
  DOC "path netcdf library")

if(NetCDF_INCLUDE_DIR AND NetCDF_LIBRARY)
  set(NetCDF_FOUND TRUE)
endif()

set(NetCDF_VERSION ${PC_NetCDF_VERSION})
set(NetCDF_INCLUDE_DIRS ${NetCDF_INCLUDEDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  NetCDF
  REQUIRED_VARS NetCDF_LIBRARY NetCDF_INCLUDEDIR
  VERSION_VAR NetCDF_VERSION)

if(NetCDF_FOUND)
  add_library(NetCDF::NetCDF UNKNOWN IMPORTED)
  set_target_properties(
    NetCDF::NetCDF
    PROPERTIES IMPORTED_LOCATION "${NetCDF_LIBRARY}"
               INTERFACE_INCLUDE_DIRECTORIES "${NetCDF_INCLUDEDIR}")
endif()
