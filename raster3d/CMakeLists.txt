build_program_in_subdir(r3.cross.rast DEPENDS grass_gis grass_raster3d)

add_subdirectory(r3.flow)

build_program_in_subdir(r3.gradient DEPENDS grass_gis grass_raster3d)

build_program_in_subdir(r3.gwflow DEPENDS grass_gis grass_raster3d grass_gpde)

build_program_in_subdir(r3.in.ascii DEPENDS grass_gis grass_raster3d)

build_program_in_subdir(r3.in.bin DEPENDS grass_gis grass_raster3d)

build_program_in_subdir(r3.info DEPENDS grass_gis grass_raster3d)

build_program_in_subdir(r3.mask DEPENDS grass_gis grass_raster3d)

build_program_in_subdir(r3.null DEPENDS grass_gis grass_raster3d)

build_program_in_subdir(r3.neighbors DEPENDS grass_gis grass_raster3d
                        grass_stats)

build_program_in_subdir(r3.out.ascii DEPENDS grass_gis grass_raster3d)

build_program_in_subdir(r3.out.bin DEPENDS grass_gis grass_raster3d)

if(NOT MSVC)
  build_program_in_subdir(r3.out.v5d DEPENDS grass_gis grass_raster3d)
endif()

build_program_in_subdir(r3.retile DEPENDS grass_gis grass_raster3d)

build_program_in_subdir(r3.stats DEPENDS grass_gis grass_raster3d)

build_program_in_subdir(r3.support DEPENDS grass_gis grass_raster3d)

build_program_in_subdir(r3.timestamp DEPENDS grass_gis grass_raster3d)

build_program_in_subdir(r3.to.rast DEPENDS grass_gis grass_raster3d)

build_program_in_subdir(r3.out.vtk DEPENDS grass_gis grass_raster3d)

build_program_in_subdir(r3.mkdspf DEPENDS grass_gis grass_raster3d grass_gpde
                        grass_dspf)

build_program_in_subdir(
  r3.in.lidar
  DEPENDS
  grass_gis
  grass_raster3d
  grass_segment
  PRIMARY_DEPENDS
  LIBLAS)

build_program_in_subdir(r3.out.netcdf DEPENDS grass_gis grass_raster3d
                        PRIMARY_DEPENDS NETCDF)

# mode_t is not available on msvc. Discuss
if(NOT MSVC)
  build_program_in_subdir(r3.in.v5d DEPENDS grass_gis grass_raster3d)
endif()
