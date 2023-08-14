build_program_in_subdir(
  ram
  DEPENDS
  grass_gis
  grass_raster
  grass_btree2
  grass_gmath
  RUNTIME_OUTPUT_DIR
  etc/r.watershed)

build_program_in_subdir(
  seg
  DEPENDS
  grass_gis
  grass_raster
  grass_gmath
  grass_segment
  RUNTIME_OUTPUT_DIR
  etc/r.watershed)

build_program_in_subdir(front NAME r.watershed DEPENDS grass_gis grass_raster)
