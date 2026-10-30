build_library_in_subdir(
    data
    NAME grass_interpdata
    DEPENDS grass_gis
    HEADERS "dataquad.h"
)

build_library_in_subdir(
    qtree
    NAME grass_qtree
    DEPENDS grass_gis
    HEADERS "qtree.h"
)

build_library_in_subdir(
    interp_float
    NAME grass_interpfl
    DEPENDS
        ${LIBM}
        grass_bitmap
        grass_dbmibase
        grass_dbmiclient
        grass_dbmidriver
        grass_gis
        grass_gmath
        grass_interpdata
        grass_qtree
        grass_raster
        grass_vector
    OPTIONAL_DEPENDS OpenMP::OpenMP_C
    HEADERS "interpf.h"
)

target_include_directories(
    grass_interpfl
    INTERFACE $<TARGET_PROPERTY:GDAL::GDAL,INTERFACE_INCLUDE_DIRECTORIES>
)
if(TARGET PostgreSQL::PostgreSQL)
    target_include_directories(
        grass_interpfl
        INTERFACE
            $<TARGET_PROPERTY:PostgreSQL::PostgreSQL,INTERFACE_INCLUDE_DIRECTORIES>
    )
endif()
