set(r3_flow_SRCS
    main.c
    flowline.c
    integrate.c
    interpolate.c
    voxel_traversal.c
)

build_program(
    NAME r3.flow
    SOURCES "${r3_flow_SRCS}"
    DEPENDS
        ${LIBM}
        grass_dbmibase
        grass_dbmiclient
        grass_dbmidriver
        grass_gis
        grass_raster
        grass_raster3d
        grass_vector
)

set(test_r3flow_SRCS
    test_main.c
    flowline.c
    integrate.c
    interpolate.c
    voxel_traversal.c
)

build_program(
    NAME test.r3flow
    SOURCES "${test_r3flow_SRCS}"
    DEPENDS
        ${LIBM}
        grass_dbmibase
        grass_dbmiclient
        grass_dbmidriver
        grass_gis
        grass_raster
        grass_raster3d
        grass_vector
)
