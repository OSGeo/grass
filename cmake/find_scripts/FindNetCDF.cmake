find_path(
  NetCDF_INCLUDE_DIR
  NAMES netcdf.h
  DOC "path to netcdf.h")

find_library(
  NetCDF_LIBRARY
  NAMES netcdf
  DOC "path netcdf library")

if(NetCDF_INCLUDE_DIR AND NetCDF_LIBRARY)
  set(NetCDF_FOUND TRUE)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NetCDF REQUIRED_VARS NetCDF_LIBRARY
                                                       NetCDF_INCLUDE_DIR)
