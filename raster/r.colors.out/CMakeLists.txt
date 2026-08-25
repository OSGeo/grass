build_program(
    NAME r.colors.out
    SOURCES raster_main.c
    DEPENDS grass_gis grass_parson grass_raster
)

build_program(
    NAME r3.colors.out
    SOURCES raster3d_main.c
    DEPENDS grass_gis grass_parson grass_raster grass_raster3d
)
