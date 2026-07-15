#[[
COPYRIGHT: (c) 2025 Nicklas Larsson and the GRASS Development Team
SPDX-License-Identifier: GPL-2.0-or-later
]]

#[=======================================================================[.rst:
FindLAPACKE
-----------

Find LAPACKE library, the C Interface to LAPACK.

.. _`The LAPACKE C Interface to LAPACK`: https://www.netlib.org/lapack/lapacke.html

Input Variables
^^^^^^^^^^^^^^^

The following variables may be set to influence this module's behavior:

``LAPACKE_PREFER_PKGCONFIG``

  If set ``pkg-config`` will be used to search for a LAPACKE library
  and if one is found that is used.
  Note: this is currently the only method.

``LAPACKE_PKGCONFIG``

  If set, the ``pkg-config`` method will look for this module name. If not
  set ``lapacke``and ``openblas`` will be looked for (in that order).

Imported targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` targets:

``LAPACKE::LAPACKE``

  The libraries to use for LAPACKE, if found.

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables:

``LAPACKE_FOUND``
  library implementing the LAPACKE interface is found
``LAPACKE_LINKER_FLAGS``
  uncached list of required linker flags (excluding ``-l`` and ``-L``).
``LAPACKE_LIBRARIES``
  list of libraries (using full path name) to link against to use LAPACKE
``LAPACKE_INCLUDE_DIRS``
  path to the LAPACKE include directory.
``LAPACKE_VERSION``
  version of the module providing LAPACKE

#]=======================================================================]

set(_default_pkgs lapacke openblas)

set(LAPACKE_FOUND)
set(LAPACKE_LIBRARIES)
set(LAPACKE_INCLUDEDIR)
set(LAPACKE_INCLUDE_DIRS)
set(LAPACKE_VERSION)
set(LAPACKE_LINKER_FLAGS)
set(HAVE_LAPACKE_DGESV)

macro(_test_package)
    # gersemi: ignore
  pkg_check_modules(PKGC_LAPACKE QUIET ${LAPACKE_PKGCONFIG})
  if(PKGC_LAPACKE_FOUND)
    set(_lapacke_found ${PKGC_LAPACKE_FOUND})
    set(_lapacke_libraries "${PKGC_LAPACKE_LINK_LIBRARIES}")
    if(MSVC)
      # MSVC needs msvc/lapacke.h to define custom complex types
      set(_lapacke_includedir "${CMAKE_SOURCE_DIR}/msvc;${PKGC_LAPACKE_INCLUDEDIR}")
    else()
      set(_lapacke_includedir "${PKGC_LAPACKE_INCLUDEDIR}")
    endif()
    set(_lapacke_include_dirs ${PKGC_LAPACKE_INCLUDEDIR}
                              ${PKGC_LAPACKE_INCLUDE_DIRS})
    set(_lapacke_version ${PKGC_LAPACKE_VERSION})
    set(_lapacke_linker_flags "${PKGC_LAPACKE_LDFLAGS}")

    list(REMOVE_DUPLICATES _lapacke_include_dirs)
    list(FILTER _lapacke_linker_flags EXCLUDE REGEX "^-L\.*|^-l\.*")
  endif()

  include(CheckSymbolExists)
  set(CMAKE_REQUIRED_LIBRARIES ${_lapacke_libraries})
  set(CMAKE_REQUIRED_INCLUDES ${_lapacke_includedir})
  set(CMAKE_REQUIRED_QUIET ON)
  check_symbol_exists(LAPACKE_dgesv "lapacke.h" HAVE_LAPACKE_DGESV)
  unset(CMAKE_REQUIRED_LIBRARIES)
  unset(CMAKE_REQUIRED_INCLUDES)
  unset(CMAKE_REQUIRED_QUIET)

  set(LAPACKE_FOUND ${_lapacke_found})
  set(LAPACKE_LIBRARIES ${_lapacke_libraries})
  set(LAPACKE_INCLUDEDIR ${_lapacke_includedir})
  set(LAPACKE_INCLUDE_DIRS ${_lapacke_include_dirs})
  set(LAPACKE_VERSION ${_lapacke_version})
  set(LAPACKE_LINKER_FLAGS ${_lapacke_linker_flags})
endmacro()

macro(_search_lapacke_pkgs)
  foreach(_pkg ${_default_pkgs})
    pkg_check_modules(PKGC_LAPACKE QUIET ${_pkg})
    if(PKGC_LAPACKE_FOUND)
      set(LAPACKE_PKGCONFIG ${_pkg})
      _test_package()
      if(HAVE_LAPACKE_DGESV)
        break()
      endif()
    endif()
  endforeach()
endmacro()

if(LAPACKE_PREFER_PKGCONFIG)
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    if(LAPACKE_PKGCONFIG)
      _test_package()
    else()
      _search_lapacke_pkgs()
    endif()
  endif()
endif()

unset(_default_pkgs)

if(NOT HAVE_LAPACKE_DGESV)
  message(STATUS "Looking for LAPACKE_dgesv - not found")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  LAPACKE
  REQUIRED_VARS LAPACKE_FOUND LAPACKE_LIBRARIES LAPACKE_INCLUDEDIR
                HAVE_LAPACKE_DGESV
  VERSION_VAR LAPACKE_VERSION)
mark_as_advanced(LAPACKE_LIBRARIES LAPACKE_INCLUDEDIR)

message(STATUS "Using LAPACKE package: ${LAPACKE_PKGCONFIG}")

if(LAPACKE_FOUND AND NOT TARGET LAPACKE::LAPACKE)
  add_library(LAPACKE::LAPACKE INTERFACE IMPORTED)
  set_target_properties(
    LAPACKE::LAPACKE
    PROPERTIES INTERFACE_LINK_LIBRARIES "${LAPACKE_LIBRARIES}"
               INTERFACE_INCLUDE_DIRECTORIES "${LAPACKE_INCLUDEDIR}")
  if(LAPACKE_LINKER_FLAGS)
    set_target_properties(LAPACKE::LAPACKE PROPERTIES INTERFACE_LINK_OPTIONS
                                                      "${LAPACKE_LINKER_FLAGS}")
  endif()
endif()
