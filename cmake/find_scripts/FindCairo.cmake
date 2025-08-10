#[[
- Try to find Cairo
Once done, this will define

  Cairo_FOUND - system has Cairo
  Cairo_INCLUDE_DIRS - the Cairo include directories
  Cairo_LIBRARIES - link these to use Cairo

Copyright (C) 2012 Raphael Kubo da Costa <rakuco@webkit.org>
Copyright (C) 2025 Nicklas Larsson

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1.  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
2.  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND ITS CONTRIBUTORS ``AS
IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ITS
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#]]

find_package(PkgConfig QUIET)
pkg_check_modules(PC_CAIRO cairo QUIET)

set(_include_dirs)
set(_link_libraries)
set(_library_dirs)
if(PkgConfig_FOUND)
  set(_modules ft fc pdf ps svg)
  if(WITH_X11)
    list(APPEND _modules xlib xlib-xrender)
  endif()
  foreach(_module ${_modules})
    pkg_check_modules(PC_cairo-${_module} cairo-${_module} QUIET)
    list(APPEND _include_dirs ${PC_cairo-${_module}_INCLUDE_DIRS})
    list(APPEND _link_libraries ${PC_cairo-${_module}_LIBRARIES})
    list(APPEND _library_dirs ${PC_cairo-${_module}_LIBRARY_DIRS})
  endforeach()
endif()

find_path(
  Cairo_INCLUDE_DIR
  NAMES cairo.h
  HINTS ${PC_CAIRO_INCLUDEDIR} ${PC_CAIRO_INCLUDE_DIRS}
  PATH_SUFFIXES cairo)

find_library(
  Cairo_LIBRARY_RELEASE
  NAMES cairo
  HINTS ${PC_CAIRO_LIBDIR} ${PC_CAIRO_LIBRARY_DIRS})

find_library(
  Cairo_LIBRARY_DEBUG
  NAMES cairod
  HINTS ${PC_CAIRO_LIBDIR} ${PC_CAIRO_LIBRARY_DIRS})

set(Cairo_LIBRARY)
if(Cairo_LIBRARY_DEBUG)
  set(Cairo_LIBRARY ${Cairo_LIBRARY_DEBUG})
elseif(Cairo_LIBRARY_RELEASE)
  set(Cairo_LIBRARY ${Cairo_LIBRARY_RELEASE})
endif()

if(Cairo_INCLUDE_DIR)
  if(EXISTS "${Cairo_INCLUDE_DIR}/cairo-version.h")
    file(READ "${Cairo_INCLUDE_DIR}/cairo-version.h" Cairo_VERSION_CONTENT)

    string(REGEX MATCH "#define +CAIRO_VERSION_MAJOR +([0-9]+)" _dummy
                 "${Cairo_VERSION_CONTENT}")
    set(Cairo_VERSION_MAJOR "${CMAKE_MATCH_1}")

    string(REGEX MATCH "#define +CAIRO_VERSION_MINOR +([0-9]+)" _dummy
                 "${Cairo_VERSION_CONTENT}")
    set(Cairo_VERSION_MINOR "${CMAKE_MATCH_1}")

    string(REGEX MATCH "#define +CAIRO_VERSION_MICRO +([0-9]+)" _dummy
                 "${Cairo_VERSION_CONTENT}")
    set(Cairo_VERSION_MICRO "${CMAKE_MATCH_1}")

    set(Cairo_VERSION
        "${Cairo_VERSION_MAJOR}.${Cairo_VERSION_MINOR}.${Cairo_VERSION_MICRO}")
  endif()
endif()

list(REMOVE_DUPLICATES _library_dirs)
set(Cairo_INCLUDE_DIRS ${Cairo_INCLUDE_DIR} ${_include_dirs})
list(REMOVE_DUPLICATES Cairo_INCLUDE_DIRS)
set(Cairo_LIBRARIES ${Cairo_LIBRARY} ${_link_libraries})
list(REMOVE_DUPLICATES Cairo_LIBRARIES)

mark_as_advanced(Cairo_INCLUDE_DIRS Cairo_LIBRARY Cairo_LIBRARY_RELEASE
                 Cairo_LIBRARY_DEBUG)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Cairo
  REQUIRED_VARS Cairo_INCLUDE_DIR Cairo_LIBRARY
  VERSION_VAR Cairo_VERSION)

if(Cairo_FOUND AND NOT TARGET Cairo::Cairo)
  add_library(Cairo::Cairo UNKNOWN IMPORTED)
  set_target_properties(
    Cairo::Cairo
    PROPERTIES IMPORTED_LOCATION "${Cairo_LIBRARY}"
               INTERFACE_INCLUDE_DIRECTORIES "${Cairo_INCLUDE_DIR}"
               INTERFACE_LINK_LIBRARIES "${Cairo_LIBRARIES}"
               INTERFACE_LINK_DIRECTORIES "${_library_dirs}")
endif()

unset(_include_dirs)
unset(_link_libraries)
unset(_library_dirs)

set(CMAKE_REQUIRED_INCLUDES ${Cairo_INCLUDE_DIRS})
include(CheckSymbolExists)
check_symbol_exists(CAIRO_HAS_XLIB_XRENDER_SURFACE "cairo.h" CAIRO_HAS_XLIB_XRENDER_SURFACE)
mark_as_advanced(CAIRO_HAS_XLIB_XRENDER_SURFACE)
unset(CMAKE_REQUIRED_INCLUDES)
