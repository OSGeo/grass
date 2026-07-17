build_library_in_subdir(
    simlib
    NAME grass_sim
    DEPENDS
        ${LIBM}
        grass_datetime
        grass_gis
        grass_gmath
        grass_raster
        grass_vector
    OPTIONAL_DEPENDS OpenMP::OpenMP_C
    HEADERS "simlib.h"
)

build_program_in_subdir(
    r.sim.water
    DEPENDS grass_gis grass_gmath grass_raster grass_sim grass_vector
    OPTIONAL_DEPENDS OpenMP::OpenMP_C
    INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/../simlib"
)

build_program_in_subdir(
    r.sim.sediment
    DEPENDS grass_gis grass_gmath grass_raster grass_sim grass_vector
    OPTIONAL_DEPENDS OpenMP::OpenMP_C
    INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/../simlib"
)
