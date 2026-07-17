build_library_in_subdir(
    lib
    NAME grass_iortho
    DEPENDS ${LIBM} grass_gis grass_gmath grass_imagery
)

build_program_in_subdir(
    i.ortho.camera
    DEPENDS grass_gis grass_gmath grass_imagery grass_iortho
)

build_program_in_subdir(
    i.ortho.elev
    DEPENDS grass_gis grass_gmath grass_imagery grass_iortho
)

build_program_in_subdir(
    i.ortho.init
    DEPENDS grass_gis grass_gmath grass_imagery grass_iortho grass_raster
)

build_program_in_subdir(
    i.ortho.photo
    DEPENDS grass_gis grass_imagery grass_iortho
)

build_program_in_subdir(
    i.ortho.rectify
    DEPENDS
        ${LIBM}
        grass_gis
        grass_gmath
        grass_imagery
        grass_iortho
        grass_raster
)

build_program_in_subdir(
    i.ortho.target
    DEPENDS grass_gis grass_imagery grass_iortho
)

build_program_in_subdir(
    i.ortho.transform
    DEPENDS ${LIBM} grass_gis grass_gmath grass_imagery grass_iortho
)
