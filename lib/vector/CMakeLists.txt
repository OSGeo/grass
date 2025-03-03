build_library_in_subdir(rtree HEADERS "rtree.h" DEPENDS grass_gis)

add_subdirectory(dglib)
add_subdirectory(diglib)

build_library_in_subdir(
  Vlib
  NAME
  grass_vector
  DEPENDS
  GDAL
  grass_btree2
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_dig2
  grass_gis
  grass_gproj
  grass_dgl
  grass_linkm
  grass_raster
  grass_rtree
  OPTIONAL_DEPENDS
  GEOS::geos_c)

if(TARGET POSTGRES)
  # target_include_directories(grass_vector SYSTEM PUBLIC
  # ${PostgreSQL_INCLUDE_DIR})
  target_link_libraries(grass_vector PRIVATE POSTGRES)
endif()

if(TARGET LAPACKE)
  target_link_libraries(grass_vector PRIVATE LAPACKE)
endif()

build_library_in_subdir(
  vedit
  NAME
  grass_vedit
  DEPENDS
  grass_gis
  grass_vector
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  GDAL)

build_library_in_subdir(
  neta
  NAME
  grass_neta
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_dgl
  grass_vector
  GDAL)

if(WITH_DOCS)
  generate_html(TARGET grass_vector NAME vectorascii)
endif()
