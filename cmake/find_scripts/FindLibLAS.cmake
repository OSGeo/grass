find_path(
  LibLAS_INCLUDE_DIR
  NAMES liblas.h
  PATH_SUFFIXES capi
  PATH_SUFFIXES liblas/capi
  DOC "path to liblas.h")

find_library(
  LibLAS_C_LIBRARY
  NAMES liblas_c las_c las
  # Help the user find it if we cannot.
  DOC "path liblas_c library")

if(LibLAS_INCLUDE_DIR)
  unset(las_version_CONTENTS)
  file(READ "${LibLAS_INCLUDE_DIR}/las_version.h" las_version_CONTENTS)

  string(REGEX MATCH "#define +LIBLAS_VERSION_MAJOR +([0-9]+)" _dummy
               "${las_version_CONTENTS}")
  set(LibLAS_VERSION_MAJOR "${CMAKE_MATCH_1}")
  string(REGEX MATCH "#define +LIBLAS_VERSION_MINOR +([0-9])" _dummy
               "${las_version_CONTENTS}")
  set(LibLAS_VERSION_MINOR "${CMAKE_MATCH_1}")
  string(REGEX MATCH "#define +LIBLAS_VERSION_REV +([0-9])" _dummy
               "${las_version_CONTENTS}")
  set(LIBLAS_VERSION_REV "${CMAKE_MATCH_1}")

  set(LibLAS_VERSION_STRING
      "${LibLAS_VERSION_MAJOR}.${LibLAS_VERSION_MINOR}.${LIBLAS_VERSION_REV}")
endif()
# message(FATAL_ERROR "LibLAS_LIBRARY=${LibLAS_LIBRARY}")
if(LibLAS_INCLUDE_DIR AND LibLAS_C_LIBRARY)
  set(LibLAS_FOUND TRUE)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  LibLAS
  REQUIRED_VARS LibLAS_C_LIBRARY LibLAS_INCLUDE_DIR
  VERSION_VAR LibLAS_VERSION_STRING)
