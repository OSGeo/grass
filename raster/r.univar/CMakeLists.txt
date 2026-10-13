set(r_univar_SRCS r.univar_main.c sort.c stats.c)
set(r3_univar_SRCS r3.univar_main.c sort.c stats.c)

build_program(
    NAME r.univar
    SOURCES "${r_univar_SRCS}"
    DEPENDS ${LIBM} grass_gis grass_parson grass_raster
    OPTIONAL_DEPENDS OpenMP::OpenMP_C
)

build_program(
    NAME r3.univar
    SOURCES "${r3_univar_SRCS}"
    DEPENDS ${LIBM} grass_gis grass_parson grass_raster grass_raster3d
    OPTIONAL_DEPENDS OpenMP::OpenMP_C
)
