#[[
COPYRIGHT: (c) 2025 Nicklas Larsson and the GRASS Development Team
SPDX-License-Identifier: GPL-2.0-or-later
]]

#[=======================================================================[.rst:
FindCBLAS
---------

Find CBLAS library, the C Interface to Basic Linear Algebra Subprograms (BLAS)

.. _`BLAS linear-algebra interface`: https://netlib.org/blas/

Input Variables
^^^^^^^^^^^^^^^

The following variables may be set to influence this module's behavior:

``CBLAS_PREFER_PKGCONFIG``

  If set, ``pkg-config`` will be used to search for a CBLAS library
  and if one is found that is used.
  Note: this is currently the only method.

``CBLAS_PKGCONFIG``

  If set, the ``pkg-config`` method will look for this module name. If not
  set ``cblas``, ``blas-netlib``, ``openblas`` and ``blas-atlas`` will be
  looked for (in that order).

Imported targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` targets:

``CBLAS::CBLAS``

  The libraries to use for CBLAS, if found.

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables:

``CBLAS_FOUND``
  library implementing the CBLAS interface is found
``CBLAS_LINKER_FLAGS``
  uncached list of required linker flags (excluding ``-l`` and ``-L``).
``CBLAS_LIBRARIES``
  list of libraries (using full path name) to link against to use CBLAS
``CBLAS_INCLUDE_DIRS``
  path to the CBLAS include directory.
``CBLAS_VERSION``
  version of the module providing CBLAS

#]=======================================================================]

set(_default_pkgs cblas blas-netlib openblas blas-atlas)

macro(_search_cblas_pkgs)
  foreach(_pkg ${_default_pkgs})
    pkg_check_modules(PKGC_CBLAS QUIET ${_pkg})
    if(PKGC_CBLAS_FOUND)
      set(CBLAS_PKGCONFIG ${_pkg})
    endif()
  endforeach()
endmacro()

if(CBLAS_PREFER_PKGCONFIG)
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    if(NOT CBLAS_PKGCONFIG)
      _search_cblas_pkgs()
    endif()
    pkg_check_modules(PKGC_CBLAS QUIET ${CBLAS_PKGCONFIG})
    if(PKGC_CBLAS_FOUND)
      set(CBLAS_FOUND ${PKGC_CBLAS_FOUND})
      set(CBLAS_LIBRARIES "${PKGC_CBLAS_LINK_LIBRARIES}")
      set(CBLAS_INCLUDEDIR "${PKGC_CBLAS_INCLUDEDIR}")
      set(CBLAS_INCLUDE_DIRS ${PKGC_CBLAS_INCLUDEDIR}
                             ${PKGC_CBLAS_INCLUDE_DIRS})
      set(CBLAS_VERSION "${PKGC_CBLAS_VERSION}")
      set(CBLAS_LINKER_FLAGS "${PKGC_CBLAS_LDFLAGS}")

      list(REMOVE_DUPLICATES CBLAS_INCLUDE_DIRS)
      list(FILTER CBLAS_LINKER_FLAGS EXCLUDE REGEX "^-L\.*|^-l\.*")
    endif()
  endif()
endif()

unset(_default_pkgs)

include(CheckSymbolExists)
set(CMAKE_REQUIRED_LIBRARIES ${CBLAS_LIBRARIES})
set(CMAKE_REQUIRED_INCLUDES ${CBLAS_INCLUDEDIR})
set(CMAKE_REQUIRED_QUIET ${CBLAS_FIND_QUIETLY})
check_symbol_exists(cblas_dgemm "cblas.h" HAVE_CBLAS_DGEMM)
unset(CMAKE_REQUIRED_LIBRARIES)
unset(CMAKE_REQUIRED_INCLUDES)
unset(CMAKE_REQUIRED_QUIET)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  CBLAS
  REQUIRED_VARS CBLAS_FOUND CBLAS_LIBRARIES CBLAS_INCLUDEDIR HAVE_CBLAS_DGEMM
  VERSION_VAR CBLAS_VERSION)
mark_as_advanced(CBLAS_LIBRARIES CBLAS_INCLUDEDIR)

if(CBLAS_FOUND AND NOT TARGET CBLAS::CBLAS)
  add_library(CBLAS::CBLAS INTERFACE IMPORTED)
  set_target_properties(
    CBLAS::CBLAS PROPERTIES INTERFACE_LINK_LIBRARIES "${CBLAS_LIBRARIES}"
                            INTERFACE_INCLUDE_DIRECTORIES "${CBLAS_INCLUDEDIR}")
  if(CBLAS_LINKER_FLAGS)
    set_target_properties(CBLAS::CBLAS PROPERTIES INTERFACE_LINK_OPTIONS
                                                  "${CBLAS_LINKER_FLAGS}")
  endif()
endif()
