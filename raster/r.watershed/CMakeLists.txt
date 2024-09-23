build_program_in_subdir(
  ram
  DEPENDS
  grass_gis
  grass_raster
  grass_btree2
  grass_gmath
  ${LIBM}
  RUNTIME_OUTPUT_DIR
  ${GRASS_INSTALL_ETCBINDIR}/r.watershed
  NO_DOCS)

build_program_in_subdir(
  seg
  DEPENDS
  grass_gis
  grass_raster
  grass_gmath
  grass_segment
  ${LIBM}
  RUNTIME_OUTPUT_DIR
  ${GRASS_INSTALL_ETCBINDIR}/r.watershed
  NO_DOCS)

build_program_in_subdir(front NAME r.watershed DEPENDS grass_gis grass_raster)
