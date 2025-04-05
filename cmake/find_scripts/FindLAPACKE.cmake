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

``CBLAS_PREFER_PKGCONFIG``

  If set ``pkg-config`` will be used to search for a CBLAS library
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
  path to the CBLAS include directory.
``LAPACKE_VERSION``
  version of the module providing LAPACKE

#]=======================================================================]

set(_default_pkgs lapacke openblas)

macro(_search_lapacke_pkgs)
  foreach(_pkg ${_default_pkgs})
    pkg_check_modules(PKGC_LAPACKE QUIET ${_pkg})
    if(PKGC_LAPACKE_FOUND)
      set(LAPACKE_PKGCONFIG ${_pkg})
    endif()
  endforeach()
endmacro()

if(CBLAS_PREFER_PKGCONFIG)
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    if(NOT LAPACKE_PKGCONFIG)
      _search_lapacke_pkgs()
    endif()
    pkg_check_modules(PKGC_LAPACKE QUIET ${LAPACKE_PKGCONFIG})
    if(PKGC_LAPACKE_FOUND)
      set(LAPACKE_FOUND ${PKGC_LAPACKE_FOUND})
      set(LAPACKE_LIBRARIES "${PKGC_LAPACKE_LINK_LIBRARIES}")
      set(LAPACKE_INCLUDEDIR "${PKGC_LAPACKE_INCLUDEDIR}")
      set(LAPACKE_INCLUDE_DIRS ${PKGC_LAPACKE_INCLUDEDIR}
                               ${PKGC_LAPACKE_INCLUDE_DIRS})
      set(LAPACKE_VERSION ${PKGC_LAPACKE_VERSION})
      set(LAPACKE_LINKER_FLAGS "${PKGC_LAPACKE_LDFLAGS}")

      list(REMOVE_DUPLICATES LAPACKE_INCLUDE_DIRS)
      list(FILTER LAPACKE_LINKER_FLAGS EXCLUDE REGEX "^-L\.*|^-l\.*")
    endif()
  endif()
endif()

unset(_default_pkgs)

include(CheckSymbolExists)
set(CMAKE_REQUIRED_LIBRARIES ${LAPACKE_LIBRARIES})
set(CMAKE_REQUIRED_INCLUDES ${LAPACKE_INCLUDEDIR})
set(CMAKE_REQUIRED_QUIET ${LAPACKE_FIND_QUIETLY})
check_symbol_exists(LAPACKE_dgesv "lapacke.h" HAVE_LAPACKE_DGESV)
unset(CMAKE_REQUIRED_LIBRARIES)
unset(CMAKE_REQUIRED_INCLUDES)
unset(CMAKE_REQUIRED_QUIET)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  LAPACKE
  REQUIRED_VARS LAPACKE_FOUND LAPACKE_LIBRARIES LAPACKE_INCLUDEDIR
                HAVE_LAPACKE_DGESV
  VERSION_VAR LAPACKE_VERSION)
mark_as_advanced(LAPACKE_LIBRARIES LAPACKE_INCLUDEDIR)

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
