build_program(
    NAME cell
    SOURCES "cell.c"
    DEPENDS grass_dbmibase grass_gis grass_raster grass_vector
    RUNTIME_OUTPUT_DIR "${GRASS_INSTALL_ETCDIR}/lister"
    NO_DOCS
)

build_program(
    NAME vector
    SOURCES "vector.c"
    DEPENDS grass_dbmibase grass_gis grass_raster grass_vector
    RUNTIME_OUTPUT_DIR "${GRASS_INSTALL_ETCDIR}/lister"
    NO_DOCS
)
