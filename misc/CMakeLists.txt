build_program_in_subdir(m.cogo DEPENDS grass_gis ${LIBM})

build_program_in_subdir(m.measure DEPENDS grass_gis)

build_program_in_subdir(m.transform DEPENDS grass_gis grass_imagery
                        grass_raster grass_vector ${LIBM})

if(TARGET grass_ogsf)
  build_program_in_subdir(
    m.nviz.image
    DEPENDS
    grass_gis
    grass_ogsf
    grass_nviz
    grass_vector
    grass_dbmibase
    grass_dbmiclient
    grass_dbmidriver)

  build_program_in_subdir(m.nviz.script DEPENDS grass_display grass_raster
                          grass_gis ${LIBM})
endif(TARGET grass_ogsf)
