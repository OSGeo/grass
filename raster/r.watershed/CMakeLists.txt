build_program_in_subdir(
    ram
    DEPENDS ${LIBM} grass_btree2 grass_gis grass_gmath grass_raster
    RUNTIME_OUTPUT_DIR ${GRASS_INSTALL_ETCBINDIR}/r.watershed
    NO_DOCS
)

build_program_in_subdir(
    seg
    DEPENDS ${LIBM} grass_gis grass_gmath grass_raster grass_segment
    RUNTIME_OUTPUT_DIR ${GRASS_INSTALL_ETCBINDIR}/r.watershed
    NO_DOCS
)

build_program_in_subdir(front NAME r.watershed DEPENDS grass_gis grass_raster)
