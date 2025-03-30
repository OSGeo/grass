set(raster3d_modules_list
    r3.cross.rast
    r3.flow
    r3.gradient
    r3.gwflow
    r3.in.ascii
    r3.in.bin
    r3.in.lidar
    r3.in.v5d
    r3.info
    r3.mask
    r3.mkdspf
    r3.neighbors
    r3.null
    r3.out.ascii
    r3.out.bin
    r3.out.netcdf
    r3.out.v5d
    r3.out.vtk
    r3.retile
    r3.stats
    r3.support
    r3.timestamp
    r3.to.rast)

if(MSVC)
  list(REMOVE_ITEM raster3d_modules_list r3.in.v5d r3.out.v5d)
endif()

add_custom_target(
  ALL_RASTER3D_MODULES ALL
  DEPENDS ${raster3d_modules_list}
  COMMENT "All temporal modules are built.")

if(WITH_DOCS)
  generate_html(TARGET ALL_RASTER3D_MODULES NAME raster3dintro)
endif()

build_program_in_subdir(r3.cross.rast DEPENDS grass_gis grass_raster
                        grass_raster3d)

add_subdirectory(r3.flow)

build_program_in_subdir(r3.gradient DEPENDS grass_gis grass_raster3d ${LIBM})

build_program_in_subdir(r3.gwflow DEPENDS grass_gis grass_gmath grass_raster3d
                        grass_gpde)

build_program_in_subdir(r3.in.ascii DEPENDS grass_gis grass_gmath grass_raster
                        grass_raster3d)

build_program_in_subdir(r3.in.bin DEPENDS grass_gis grass_raster grass_raster3d)

build_program_in_subdir(r3.info DEPENDS grass_gis grass_raster grass_raster3d)

build_program_in_subdir(r3.mask DEPENDS grass_gis grass_raster3d)

build_program_in_subdir(r3.null DEPENDS grass_gis grass_raster3d)

build_program_in_subdir(r3.neighbors DEPENDS grass_gis grass_raster3d
                        grass_stats grass_raster)

build_program_in_subdir(r3.out.ascii DEPENDS grass_gis grass_raster3d)

build_program_in_subdir(r3.out.bin DEPENDS grass_gis grass_raster3d)

if(NOT MSVC)
  build_program_in_subdir(r3.out.v5d DEPENDS grass_gis grass_raster3d ${LIBM})
endif()

build_program_in_subdir(r3.retile DEPENDS grass_gis grass_raster grass_raster3d)

build_program_in_subdir(r3.stats DEPENDS grass_gis grass_raster3d)

build_program_in_subdir(r3.support DEPENDS grass_gis grass_raster
                        grass_raster3d)

build_program_in_subdir(r3.timestamp DEPENDS grass_gis grass_raster3d)

build_program_in_subdir(r3.to.rast DEPENDS grass_gis grass_raster
                        grass_raster3d)

build_program_in_subdir(r3.out.vtk DEPENDS grass_gis grass_raster
                        grass_raster3d)

build_program_in_subdir(r3.mkdspf DEPENDS grass_gis grass_raster3d grass_gpde
                        grass_dspf ${LIBM})

build_program_in_subdir(
  r3.in.lidar
  DEPENDS
  grass_gis
  grass_gproj
  grass_raster
  grass_raster3d
  grass_segment
  ${LIBM}
  PRIMARY_DEPENDS
  LIBLAS)

build_program_in_subdir(
  r3.out.netcdf
  DEPENDS
  grass_datetime
  grass_gis
  grass_gproj
  grass_raster3d
  PRIMARY_DEPENDS
  NetCDF::NetCDF
  OPTIONAL_DEPENDS
  PROJ::proj)

# mode_t is not available on msvc. Discuss
if(NOT MSVC)
  build_program_in_subdir(r3.in.v5d DEPENDS grass_gis grass_raster3d ${LIBM})
endif()
