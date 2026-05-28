#[[
  Find GEOS
  ~~~
  Copyright (c) 2008, Mateusz Loskot <mateusz@loskot.net>
  (based on FindGDAL.cmake by Magnus Homann)
  Redistribution and use is allowed according to the terms of the BSD license.
  For details see the accompanying COPYING-CMAKE-SCRIPTS file.

  CMake module to search for GEOS library

  If it's found it sets GEOS_FOUND to TRUE
  and the following target is added

      GEOS::geos_c
#]]

find_package(GEOS CONFIG)

if(NOT GEOS_FOUND)

  if(WIN32)

    if(MINGW)
      find_path(GEOS_INCLUDE_DIR geos_c.h "$ENV{LIB_DIR}/include"
                /usr/local/include /usr/include c:/msys/local/include)
      find_library(
        GEOS_LIBRARY
        NAMES geos_c
        PATHS "$ENV{LIB_DIR}/lib" /usr/local/lib /usr/lib c:/msys/local/lib)
    endif(MINGW)

    if(MSVC)
      find_path(GEOS_INCLUDE_DIR geos_c.h $ENV{LIB_DIR}/include $ENV{INCLUDE})
      find_library(
        GEOS_LIBRARY
        NAMES geos_c_i geos_c
        PATHS "$ENV{LIB_DIR}/lib" $ENV{LIB})
    endif(MSVC)

  #[[
  elseif(APPLE AND QGIS_MAC_DEPS_DIR)

    find_path(GEOS_INCLUDE_DIR geos_c.h "$ENV{LIB_DIR}/include")
    find_library(
      GEOS_LIBRARY
      NAMES geos_c
      PATHS "$ENV{LIB_DIR}/lib")
  #]]
  else(WIN32)

    if(UNIX)
      # try to use framework on mac want clean framework path, not unix
      # compatibility path
      if(APPLE)
        if(CMAKE_FIND_FRAMEWORK MATCHES "FIRST"
           OR CMAKE_FRAMEWORK_PATH MATCHES "ONLY"
           OR NOT CMAKE_FIND_FRAMEWORK)
          set(CMAKE_FIND_FRAMEWORK_save
              ${CMAKE_FIND_FRAMEWORK}
              CACHE STRING "" FORCE)
          set(CMAKE_FIND_FRAMEWORK
              "ONLY"
              CACHE STRING "" FORCE)
          find_library(GEOS_LIBRARY GEOS)
          if(GEOS_LIBRARY)
            # they're all the same in a framework
            set(GEOS_INCLUDE_DIR
                ${GEOS_LIBRARY}/Headers
                CACHE PATH "Path to a file.")
            # set GEOS_CONFIG to make later test happy, not used here, may not
            # exist
            set(GEOS_CONFIG
                ${GEOS_LIBRARY}/unix/bin/geos-config
                CACHE FILEPATH "Path to a program.")
            # version in info.plist
            get_version_plist(${GEOS_LIBRARY}/Resources/Info.plist GEOS_VERSION)
            if(NOT GEOS_VERSION)
              message(
                FATAL_ERROR "Could not determine GEOS version from framework.")
            endif(NOT GEOS_VERSION)
            string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\1"
                                 GEOS_VERSION_MAJOR "${GEOS_VERSION}")
            string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\2"
                                 GEOS_VERSION_MINOR "${GEOS_VERSION}")
            if(GEOS_VERSION_MAJOR LESS 3)
              message(
                FATAL_ERROR
                  "GEOS version is too old (${GEOS_VERSION}). Use 3.0.0 or higher."
              )
            endif(GEOS_VERSION_MAJOR LESS 3)
          endif(GEOS_LIBRARY)
          set(CMAKE_FIND_FRAMEWORK
              ${CMAKE_FIND_FRAMEWORK_save}
              CACHE STRING "" FORCE)
        endif()
      endif(APPLE)

      if(CYGWIN)
        find_library(
          GEOS_LIBRARY
          NAMES geos_c
          PATHS /usr/lib /usr/local/lib)
      endif(CYGWIN)

      if(NOT GEOS_INCLUDE_DIR
         OR NOT GEOS_LIBRARY
         OR NOT GEOS_CONFIG)
        # didn't find OS X framework, and was not set by user
        set(GEOS_CONFIG_PREFER_PATH
            "$ENV{GEOS_HOME}/bin"
            CACHE STRING "preferred path to GEOS (geos-config)")
        find_program(GEOS_CONFIG geos-config ${GEOS_CONFIG_PREFER_PATH}
                     $ENV{LIB_DIR}/bin /usr/local/bin/ /usr/bin/)
        # MESSAGE("DBG GEOS_CONFIG ${GEOS_CONFIG}")

        if(GEOS_CONFIG)

          execute_process(
            COMMAND ${GEOS_CONFIG} --version
            OUTPUT_STRIP_TRAILING_WHITESPACE
            OUTPUT_VARIABLE GEOS_VERSION)
          string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\1"
                               GEOS_VERSION_MAJOR "${GEOS_VERSION}")
          string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\2"
                               GEOS_VERSION_MINOR "${GEOS_VERSION}")

          if(GEOS_VERSION_MAJOR LESS 3 OR (GEOS_VERSION_MAJOR EQUAL 3
                                           AND GEOS_VERSION_MINOR LESS 9))
            message(
              FATAL_ERROR
                "GEOS version is too old (${GEOS_VERSION}). Use 3.9.0 or higher."
            )
          endif(GEOS_VERSION_MAJOR LESS 3 OR (GEOS_VERSION_MAJOR EQUAL 3
                                              AND GEOS_VERSION_MINOR LESS 9))

          # set INCLUDE_DIR to prefix+include
          execute_process(
            COMMAND ${GEOS_CONFIG} --prefix
            OUTPUT_STRIP_TRAILING_WHITESPACE
            OUTPUT_VARIABLE GEOS_PREFIX)

          find_path(GEOS_INCLUDE_DIR geos_c.h ${GEOS_PREFIX}/include
                    /usr/local/include /usr/include)

          # extract link dirs for rpath
          execute_process(
            COMMAND ${GEOS_CONFIG} --libs
            OUTPUT_STRIP_TRAILING_WHITESPACE
            OUTPUT_VARIABLE GEOS_CONFIG_LIBS)

          # split off the link dirs (for rpath) use regular expression to match
          # wildcard equivalent "-L*<endchar>" with <endchar> is a space or a
          # semicolon
          string(REGEX MATCHALL "[-][L]([^ ;])+"
                       GEOS_LINK_DIRECTORIES_WITH_PREFIX "${GEOS_CONFIG_LIBS}")
          # MESSAGE("DBG
          # GEOS_LINK_DIRECTORIES_WITH_PREFIX=${GEOS_LINK_DIRECTORIES_WITH_PREFIX}")

          # remove prefix -L because we need the pure directory for
          # LINK_DIRECTORIES

          if(GEOS_LINK_DIRECTORIES_WITH_PREFIX)
            string(REGEX REPLACE "[-][L]" "" GEOS_LINK_DIRECTORIES
                                 ${GEOS_LINK_DIRECTORIES_WITH_PREFIX})
          endif(GEOS_LINK_DIRECTORIES_WITH_PREFIX)

          # XXX - mloskot: geos-config --libs does not return -lgeos_c, so set
          # it manually split off the name use regular expression to match
          # wildcard equivalent "-l*<endchar>" with <endchar> is a space or a
          # semicolon STRING(REGEX MATCHALL "[-][l]([^ ;])+"
          # GEOS_LIB_NAME_WITH_PREFIX "${GEOS_CONFIG_LIBS}" ) MESSAGE("DBG
          # GEOS_CONFIG_LIBS=${GEOS_CONFIG_LIBS}") MESSAGE("DBG
          # GEOS_LIB_NAME_WITH_PREFIX=${GEOS_LIB_NAME_WITH_PREFIX}")
          set(GEOS_LIB_NAME_WITH_PREFIX
              -lgeos_c
              CACHE STRING INTERNAL)

          # remove prefix -l because we need the pure name

          if(GEOS_LIB_NAME_WITH_PREFIX)
            string(REGEX REPLACE "[-][l]" "" GEOS_LIB_NAME
                                 ${GEOS_LIB_NAME_WITH_PREFIX})
          endif(GEOS_LIB_NAME_WITH_PREFIX)
          # MESSAGE("DBG  GEOS_LIB_NAME=${GEOS_LIB_NAME}")

          if(APPLE)
            if(NOT GEOS_LIBRARY)
              # work around empty GEOS_LIBRARY left by framework check while
              # still preserving user setting if given ***FIXME*** need to
              # improve framework check so below not needed
              set(GEOS_LIBRARY
                  ${GEOS_LINK_DIRECTORIES}/lib${GEOS_LIB_NAME}.dylib
                  CACHE STRING INTERNAL FORCE)
            endif(NOT GEOS_LIBRARY)
          else(APPLE)
            find_library(
              GEOS_LIBRARY
              NAMES ${GEOS_LIB_NAME}
              PATHS ${GEOS_LIB_DIRECTORIES}/lib)
          endif(APPLE)
          # MESSAGE("DBG  GEOS_LIBRARY=${GEOS_LIBRARY}")

        else(GEOS_CONFIG)
          message(
            "FindGEOS.cmake: geos-config not found. Please set it manually. GEOS_CONFIG=${GEOS_CONFIG}"
          )
        endif(GEOS_CONFIG)
      endif(
        NOT GEOS_INCLUDE_DIR
        OR NOT GEOS_LIBRARY
        OR NOT GEOS_CONFIG)
    endif(UNIX)
  endif(WIN32)

  if(GEOS_INCLUDE_DIR AND NOT GEOS_VERSION)
    file(READ ${GEOS_INCLUDE_DIR}/geos_c.h VERSIONFILE)
    string(REGEX MATCH "#define GEOS_VERSION \"[0-9]+\\.[0-9]+\\.[0-9]+"
                 GEOS_VERSION ${VERSIONFILE})
    string(REGEX MATCH "[0-9]+\\.[0-9]\\.[0-9]+" GEOS_VERSION ${GEOS_VERSION})
  endif(GEOS_INCLUDE_DIR AND NOT GEOS_VERSION)

  if(GEOS_INCLUDE_DIR AND GEOS_LIBRARY)
    set(GEOS_FOUND TRUE)
  endif(GEOS_INCLUDE_DIR AND GEOS_LIBRARY)

  if(GEOS_FOUND)

    add_library(GEOS::geos_c UNKNOWN IMPORTED)
    target_link_libraries(GEOS::geos_c INTERFACE ${GEOS_LIBRARY})
    target_include_directories(GEOS::geos_c INTERFACE ${GEOS_INCLUDE_DIR})
    set_target_properties(GEOS::geos_c PROPERTIES IMPORTED_LOCATION
                                                  ${GEOS_LIBRARY})

    if(NOT GEOS_FIND_QUIETLY)
      message(STATUS "Found GEOS: ${GEOS_LIBRARY} (${GEOS_VERSION})")
    endif(NOT GEOS_FIND_QUIETLY)

  else(GEOS_FOUND)

    message(GEOS_INCLUDE_DIR=${GEOS_INCLUDE_DIR})
    message(GEOS_LIBRARY=${GEOS_LIBRARY})
    message(FATAL_ERROR "Could not find GEOS")

  endif(GEOS_FOUND)
endif()
