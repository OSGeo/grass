#TODO: docs vectorascii
# PGM = vectorascii

build_library_in_subdir(rtree HEADERS "rtree.h" DEPENDS grass_gis)

add_subdirectory(dglib)
add_subdirectory(diglib)

build_library_in_subdir(Vlib
  NAME grass_vector
  DEPENDS grass_gis grass_raster grass_rtree
  grass_graph grass_dig2 grass_dbmibase
  grass_btree2 grass_dbmiclient
  grass_linkm grass_gproj
  )

if(TARGET POSTGRES)
  target_link_libraries(grass_vector POSTGRES)
endif()

add_subdirectory(vedit)

add_subdirectory(neta)
