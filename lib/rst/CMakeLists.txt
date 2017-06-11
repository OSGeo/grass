#add_subdirectory(data)
#add_subdirectory(qtree)
#add_subdirectory(interp_float)

build_library_in_subdir(data
  NAME grass_interpdata
  DEPENDS grass_gis
  HEADERS "dataquad.h")

build_library_in_subdir(qtree
  NAME grass_qtree
  DEPENDS grass_gis
  HEADERS "qtree.h")


build_library_in_subdir(interp_float
  NAME grass_interpfl
  DEFS "-DPOINT2D_C=1"
  DEPENDS GEOS grass_gis grass_raster grass_bitmap grass_vector grass_qtree grass_interpdata
  HEADERS "interpf.h")

