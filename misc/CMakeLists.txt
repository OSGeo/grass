build_program_in_subdir(m.cogo DEPENDS grass_gis ${LIBM})

build_program_in_subdir(m.measure DEPENDS grass_gis grass_parson)

build_program_in_subdir(
    m.transform
    DEPENDS ${LIBM} grass_gis grass_imagery grass_raster grass_vector
)

if(TARGET grass_ogsf)
    build_program_in_subdir(
        m.nviz.image
        DEPENDS
            grass_dbmibase
            grass_dbmiclient
            grass_dbmidriver
            grass_gis
            grass_nviz
            grass_ogsf
            grass_vector
    )

    build_program_in_subdir(
        m.nviz.script
        DEPENDS ${LIBM} grass_display grass_gis grass_raster
    )
endif(TARGET grass_ogsf)
