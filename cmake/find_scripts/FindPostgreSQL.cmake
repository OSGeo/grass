#]=======================================================================]

# ----------------------------------------------------------------------------
# History:
# This module is derived from the module originally found in the VTK source tree.
#
# ----------------------------------------------------------------------------
# Note:
# PostgreSQL_ADDITIONAL_VERSIONS is a variable that can be used to set the
# version number of the implementation of PostgreSQL.
# In Windows the default installation of PostgreSQL uses that as part of the path.
# E.g C:\Program Files\PostgreSQL\8.4.
# Currently, the following version numbers are known to this module:
# "11" "10" "9.6" "9.5" "9.4" "9.3" "9.2" "9.1" "9.0" "8.4" "8.3" "8.2" "8.1" "8.0"
#
# To use this variable just do something like this:
# set(PostgreSQL_ADDITIONAL_VERSIONS "9.2" "8.4.4")
# before calling find_package(PostgreSQL) in your CMakeLists.txt file.
# This will mean that the versions you set here will be found first in the order
# specified before the default ones are searched.
#
# ----------------------------------------------------------------------------
# You may need to manually set:
#  PostgreSQL_INCLUDE_DIR  - the path to where the PostgreSQL include files are.
#  PostgreSQL_LIBRARY_DIR  - The path to where the PostgreSQL library files are.
# If FindPostgreSQL.cmake cannot find the include files or the library files.
#
# ----------------------------------------------------------------------------
# The following variables are set if PostgreSQL is found:
#  PostgreSQL_FOUND         - Set to true when PostgreSQL is found.
#  PostgreSQL_INCLUDE_DIRS  - Include directories for PostgreSQL
#  PostgreSQL_LIBRARY_DIRS  - Link directories for PostgreSQL libraries
#  PostgreSQL_LIBRARIES     - The PostgreSQL libraries.
#
# The ``PostgreSQL::PostgreSQL`` imported target is also created.
#
# ----------------------------------------------------------------------------
# If you have installed PostgreSQL in a non-standard location.
# (Please note that in the following comments, it is assumed that <Your Path>
# points to the root directory of the include directory of PostgreSQL.)
# Then you have three options.
# 1) After CMake runs, set PostgreSQL_INCLUDE_DIR to <Your Path>/include and
#    PostgreSQL_LIBRARY_DIR to wherever the library pq (or libpq in windows) is
# 2) Use CMAKE_INCLUDE_PATH to set a path to <Your Path>/PostgreSQL<-version>. This will allow find_path()
#    to locate PostgreSQL_INCLUDE_DIR by utilizing the PATH_SUFFIXES option. e.g. In your CMakeLists.txt file
#    set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} "<Your Path>/include")
# 3) Set an environment variable called ${PostgreSQL_ROOT} that points to the root of where you have
#    installed PostgreSQL, e.g. <Your Path>.
#
# ----------------------------------------------------------------------------
find_path(PostgreSQL_INCLUDE_DIR
  NAMES libpq-fe.h
  PATH_SUFFIXES
    pgsql
    postgresql
    include

  # Help the user find it if we cannot.
  DOC "path to libpq-fe.h"
)

find_path(PostgreSQL_TYPE_INCLUDE_DIR
  NAMES catalog/pg_type.h
  PATH_SUFFIXES
    postgresql
    pgsql/server
    postgresql/server
    postgresql/9.5/server
    include/server

  # Help the user find it if we cannot.
  DOC "path to postgresql header catalog/pg_type.h"
)

find_library(PostgreSQL_LIBRARY
 NAMES libpqd pqd libpq pq
 # Help the user find it if we cannot.
 DOC "The ${PostgreSQL_LIBRARY_DIR_MESSAGE}")

get_filename_component(PostgreSQL_LIBRARY_DIR ${PostgreSQL_LIBRARY} PATH)

if (PostgreSQL_INCLUDE_DIR)
  # Some platforms include multiple pg_config.hs for multi-lib configurations
  # This is a temporary workaround.  A better solution would be to compile
  # a dummy c file and extract the value of the symbol.
  file(GLOB _PG_CONFIG_HEADERS "${PostgreSQL_INCLUDE_DIR}/pg_config*.h")
  foreach(_PG_CONFIG_HEADER ${_PG_CONFIG_HEADERS})
    if(EXISTS "${_PG_CONFIG_HEADER}")
      file(STRINGS "${_PG_CONFIG_HEADER}" pgsql_version_str
           REGEX "^#define[\t ]+PG_VERSION_NUM[\t ]+.*")
      if(pgsql_version_str)
        string(REGEX REPLACE "^#define[\t ]+PG_VERSION_NUM[\t ]+([0-9]*).*"
               "\\1" _PostgreSQL_VERSION_NUM "${pgsql_version_str}")
        break()
      endif()
    endif()
  endforeach()
  if (_PostgreSQL_VERSION_NUM)
    math(EXPR _PostgreSQL_major_version "${_PostgreSQL_VERSION_NUM} / 10000")
    math(EXPR _PostgreSQL_minor_version "${_PostgreSQL_VERSION_NUM} % 10000")
    set(PostgreSQL_VERSION_STRING "${_PostgreSQL_major_version}.${_PostgreSQL_minor_version}")
    unset(_PostgreSQL_major_version)
    unset(_PostgreSQL_minor_version)
  else ()
    foreach(_PG_CONFIG_HEADER ${_PG_CONFIG_HEADERS})
      if(EXISTS "${_PG_CONFIG_HEADER}")
        file(STRINGS "${_PG_CONFIG_HEADER}" pgsql_version_str
             REGEX "^#define[\t ]+PG_VERSION[\t ]+\".*\"")
        if(pgsql_version_str)
          string(REGEX REPLACE "^#define[\t ]+PG_VERSION[\t ]+\"([^\"]*)\".*"
                 "\\1" PostgreSQL_VERSION_STRING "${pgsql_version_str}")
          break()
        endif()
      endif()
    endforeach()
  endif ()
  unset(_PostgreSQL_VERSION_NUM)
  unset(pgsql_version_str)
endif()

# Did we find anything?
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PostgreSQL
                                  REQUIRED_VARS PostgreSQL_LIBRARY PostgreSQL_INCLUDE_DIR PostgreSQL_TYPE_INCLUDE_DIR
                                  VERSION_VAR PostgreSQL_VERSION_STRING)
set(PostgreSQL_FOUND  ${POSTGRESQL_FOUND})
