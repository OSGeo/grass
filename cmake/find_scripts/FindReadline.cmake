#[[
COPYRIGHT: (c) 2025 Nicklas Larsson and the GRASS Development Team
SPDX-License-Identifier: GPL-2.0-or-later
]]

#[=======================================================================[.rst:
FindReadline
------------

FindModule for the GNU Readline library.

Optional COMPONENTS
^^^^^^^^^^^^^^^^^^^

This module respects several optional COMPONENTS:

``History``
  The GNU History library.

Imported Targets
^^^^^^^^^^^^^^^^

This module defines the :prop_tgt:`IMPORTED` targets:

``Readline::Readline``
  Defined to the the GNU Readline library.
``Readline::History``
  Defined to the GNU History library.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Readline_FOUND``
True, if the system has Readline and all components are found.
``Readline_VERSION``
Readline's version.
``Readline_LIBRARIES``
Path to the Readline library.
``Readline_INCLUDE_DIRS``
Path to the Readline include directory.
``History_FOUND``
True, if the system has History.
``History_LIBRARIES``
Path to the History library.
``History_INCLUDE_DIRS``
Path to the History include directory.

Cache variables
^^^^^^^^^^^^^^^
``Readline_INCLUDE_DIR``
Path to the Readline include directory.
``Readline_LIBRARY``
Path to the Readline library.
``History_INCLUDE_DIR``
Path to the History include directory.
``History_LIBRARY``
Path to the History library.

Example of use:

.. code-block:: cmake

  find_package(Readline REQUIRED)

  find_package(Readline REQUIRED COMPONENTS History)

#]=======================================================================]

set(_Readline_REQUIRED_VARS Readline_INCLUDE_DIR Readline_LIBRARY)

foreach(component ${Readline_FIND_COMPONENTS})
  string(TOUPPER ${component} _COMPONENT)
  set(Readline_USE_${_COMPONENT} 1)
  list(APPEND _Readline_REQUIRED_VARS ${component}_INCLUDE_DIR
       ${component}_LIBRARY)
endforeach()

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_Readline QUIET "readline")
  find_path(
    Readline_INCLUDE_DIR
    NAMES readline/readline.h
    HINTS ${PC_Readline_INCLUDEDIR} ${PC_Readline_INCLUDE_DIRS})
  find_library(
    Readline_LIBRARY
    NAMES readline
    HINTS ${PC_Readline_LIBDIR} ${PC_Readline_LIBRARY_DIRS})
  set(Readline_FOUND ${PC_Readline_FOUND})
  set(Readline_VERSION ${PC_Readline_VERSION})

  if(Readline_USE_HISTORY)
    pkg_check_modules(PC_History QUIET "history")
    find_path(
      History_INCLUDE_DIR
      NAMES readline/history.h
      HINTS ${PC_History_INCLUDEDIR} ${PC_History_INCLUDE_DIRS})
    find_library(
      History_LIBRARY
      NAMES history
      HINTS ${PC_History_LIBDIR} ${PC_History_LIBRARY_DIRS})
    set(History_FOUND ${PC_History_FOUND})
    set(History_VERSION ${PC_History_VERSION})
  endif()
endif()

if(NOT Readline_FOUND)
  find_path(Readline_ROOT_DIR NAMES include/readline/readline.h)
  find_path(
    Readline_INCLUDE_DIR
    NAMES readline/readline.h
    HINTS ${Readline_ROOT_DIR}/include)
  find_library(
    Readline_LIBRARY
    NAMES readline
    HINTS ${Readline_ROOT_DIR}/lib)
  if(Readline_INCLUDE_DIR AND Readline_LIBRARY)
    set(Readline_FOUND TRUE)
    if(EXISTS "${Readline_INCLUDE_DIR}/readline/readline.h")
      file(STRINGS "${Readline_INCLUDE_DIR}/readline/readline.h"
           version_major_str REGEX "^#define[\t ]+RL_VERSION_MAJOR[\t ]+[0-9]+")
      string(REGEX REPLACE "^#define[\t ]+RL_VERSION_MAJOR[\t ]+([0-9]+)" "\\1"
                           version_major "${version_major_str}")
      file(STRINGS "${Readline_INCLUDE_DIR}/readline/readline.h"
           version_minor_str REGEX "^#define[\t ]+RL_VERSION_MINOR[\t ]+[0-9]+")
      string(REGEX REPLACE "^#define[\t ]+RL_VERSION_MINOR[\t ]+([0-9]+)" "\\1"
                           version_minor "${version_minor_str}")
      set(Readline_VERSION "${version_major}.${version_minor}")
    endif()
  endif()
endif()

if(Readline_USE_HISTORY AND NOT History_FOUND)
  find_path(History_ROOT_DIR NAMES include/readline/history.h)
  find_path(
    History_INCLUDE_DIR
    NAMES readline/history.h
    HINTS ${History_ROOT_DIR}/include)
  find_library(
    History_LIBRARY
    NAMES history
    HINTS ${History_ROOT_DIR}/lib)
  if(History_INCLUDE_DIR AND History_LIBRARY)
    set(History_FOUND TRUE)
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Readline
  REQUIRED_VARS ${_Readline_REQUIRED_VARS}
  VERSION_VAR Readline_VERSION)
mark_as_advanced(${_Readline_REQUIRED_VARS})
unset(_Readline_REQUIRED_VARS)

if(Readline_FOUND AND NOT TARGET Readline::Readline)
  set(Readline_LIBRARIES ${Readline_LIBRARY})
  set(Readline_INCLUDE_DIRS ${Readline_INCLUDE_DIR})
  add_library(Readline::Readline UNKNOWN IMPORTED)
  set_target_properties(
    Readline::Readline
    PROPERTIES IMPORTED_LOCATION "${Readline_LIBRARY}"
               INTERFACE_INCLUDE_DIRECTORIES "${Readline_INCLUDE_DIR}")
endif()

mark_as_advanced(History_INCLUDE_DIR History_LIBRARY)

if(Readline_USE_HISTORY
   AND History_FOUND
   AND NOT TARGET Readline::History)
  set(History_LIBRARIES ${History_LIBRARY})
  set(History_INCLUDE_DIRS ${History_INCLUDE_DIR})

  add_library(Readline::History UNKNOWN IMPORTED)
  set_target_properties(
    Readline::History
    PROPERTIES IMPORTED_LOCATION "${History_LIBRARY}"
               INTERFACE_INCLUDE_DIRECTORIES "${History_INCLUDE_DIR}")
endif()
