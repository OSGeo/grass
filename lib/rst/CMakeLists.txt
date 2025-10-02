build_library_in_subdir(
  data
  NAME
  grass_interpdata
  DEPENDS
  grass_gis
  HEADERS
  "dataquad.h")

build_library_in_subdir(
  qtree
  NAME
  grass_qtree
  DEPENDS
  grass_gis
  HEADERS
  "qtree.h")

build_library_in_subdir(
  interp_float
  NAME
  grass_interpfl
  DEPENDS
  grass_gis
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gmath
  grass_raster
  grass_bitmap
  grass_vector
  grass_qtree
  grass_interpdata
  ${LIBM}
  OPTIONAL_DEPENDS
  OPENMP
  HEADERS
  "interpf.h")

target_include_directories(grass_interpfl PUBLIC ${GDAL_INCLUDE_DIR})
if(TARGET PostgreSQL::PostgreSQL)
  target_include_directories(grass_interpfl PUBLIC ${PostgreSQL_INCLUDE_DIR})
endif()
