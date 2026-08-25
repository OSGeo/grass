build_library_in_subdir(
    lib
    NAME grass_lrs
    DEPENDS grass_dbmibase grass_dbmiclient grass_dbmidriver grass_gis
    HEADERS "lrs.h"
)
add_dependencies(grass_lrs db.drivers)
build_program_in_subdir(
    v.lrs.create
    DEPENDS
        ${LIBM}
        GDAL::GDAL
        grass_dbmibase
        grass_dbmiclient
        grass_dbmidriver
        grass_gis
        grass_lrs
        grass_vector
)

build_program_in_subdir(
    v.lrs.segment
    DEPENDS
        ${LIBM}
        GDAL::GDAL
        grass_dbmibase
        grass_dbmiclient
        grass_dbmidriver
        grass_gis
        grass_lrs
        grass_vector
)

build_program_in_subdir(
    v.lrs.label
    DEPENDS
        ${LIBM}
        GDAL::GDAL
        grass_dbmibase
        grass_dbmiclient
        grass_dbmidriver
        grass_gis
        grass_lrs
        grass_vector
)

build_program_in_subdir(
    v.lrs.where
    DEPENDS
        GDAL::GDAL
        grass_dbmibase
        grass_dbmiclient
        grass_dbmidriver
        grass_gis
        grass_lrs
        grass_vector
)
