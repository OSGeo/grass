set(raster_modules_list
    r.basins.fill
    r.buffer
    r.buildvrt
    r.carve
    r.category
    r.circle
    r.clump
    r.coin
    r.colors
    r.colors.out
    r.composite
    r.compress
    r.contour
    r.cost
    r.covar
    r.cross
    r.describe
    r.distance
    r.external
    r.external.out
    r.fill.dir
    r.fill.stats
    r.flow
    r.geomorphon
    r.grow.distance
    r.gwflow
    r.his
    r.horizon
    r.in.ascii
    r.in.bin
    r.in.gdal
    r.in.gridatb
    r.in.lidar
    r.in.mat
    r.in.pdal
    r.in.poly
    r.in.xyz
    r.info
    r.kappa
    r.lake
    r.latlong
    r.li
    r.mapcalc
    r.mask.status
    r.mfilter
    r.mode
    r.neighbors
    r.null
    r.object.geometry
    r.out.ascii
    r.out.bin
    r.out.gdal
    r.out.gridatb
    r.out.mat
    r.out.mpeg
    r.out.pov
    r.out.ppm
    r.out.ppm3
    r.out.vrml
    r.out.vtk
    r.param.scale
    r.patch
    r.path
    r.profile
    r.proj
    r.quant
    r.quantile
    r.random
    r.random.cells
    r.random.surface
    r.reclass
    r.recode
    r.region
    r.regression.line
    r.regression.multi
    r.relief
    r.report
    r.resamp.bspline
    r.resamp.filter
    r.resamp.interp
    r.resamp.rst
    r.resamp.stats
    r.resample
    r.rescale
    r.rescale.eq
    r.ros
    r.series
    r.series.accumulate
    r.series.interp
    r.sim
    r.slope.aspect
    r.solute.transport
    r.spread
    r.spreadpath
    r.statistics
    r.stats
    r.stats.quantile
    r.stats.zonal
    r.stream.extract
    r.sun
    r.sunhours
    r.sunmask
    r.support
    r.support.stats
    r.surf.area
    r.surf.contour
    r.surf.fractal
    r.surf.gauss
    r.surf.idw
    r.surf.random
    r.terraflow
    r.texture
    r.thin
    r.tile
    r.timestamp
    r.to.rast3
    r.to.rast3elev
    r.to.vect
    r.topidx
    r.topmodel
    r.transect
    r.univar
    r.uslek
    r.usler
    r.viewshed
    r.volume
    r.walk
    r.water.outlet
    r.watershed
    r.what
    r.what.color)

if(WITH_LIBPNG)
  list(APPEND raster_modules_list r.in.png r.out.png)
endif()

if(MSVC)
  list(REMOVE_ITEM raster_modules_list r.in.xyz)
endif()

add_custom_target(
  ALL_RASTER_MODULES ALL
  DEPENDS ${raster_modules_list}
  COMMENT "All raster modules are built.")

if(WITH_DOCS)
  generate_html(TARGET ALL_RASTER_MODULES NAME rasterintro)
endif()

build_program_in_subdir(r.basins.fill DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.buildvrt DEPENDS grass_gis grass_raster grass_gmath
                        ${LIBM})

build_program_in_subdir(r.buffer DEPENDS grass_gis grass_raster)

build_program_in_subdir(
  r.carve
  DEPENDS
  grass_gis
  grass_raster
  grass_vector
  grass_bitmap
  grass_dig2
  ${LIBM})

build_program_in_subdir(r.category DEPENDS grass_gis grass_raster grass_parson)

build_program_in_subdir(r.circle DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.clump DEPENDS grass_gis grass_raster grass_btree2)

build_program_in_subdir(r.coin DEPENDS grass_gis grass_raster)

add_subdirectory(r.colors)

add_subdirectory(r.colors.out)

build_program_in_subdir(r.composite DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.compress DEPENDS grass_gis grass_raster)

build_program_in_subdir(
  r.contour
  DEPENDS
  grass_gis
  grass_raster
  grass_vector
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  ${LIBM})

build_program_in_subdir(
  r.cost
  DEPENDS
  grass_gis
  grass_raster
  grass_segment
  grass_vector
  ${LIBM})

build_program_in_subdir(r.covar DEPENDS grass_gis grass_raster ${LIBM})

build_program_in_subdir(r.cross DEPENDS grass_gis grass_raster grass_btree
                        grass_btree2)

build_program_in_subdir(r.describe DEPENDS grass_gis grass_raster grass_parson)

build_program_in_subdir(r.distance DEPENDS grass_gis grass_raster)

build_program_in_subdir(
  r.external
  DEPENDS
  grass_gis
  grass_raster
  grass_imagery
  grass_gproj
  ${LIBM}
  PRIMARY_DEPENDS
  GDAL::GDAL)

build_program_in_subdir(r.external.out DEPENDS grass_gis grass_raster grass_gproj
                        PRIMARY_DEPENDS GDAL::GDAL)

build_program_in_subdir(r.fill.dir DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.fill.stats DEPENDS grass_gis grass_raster ${LIBM})

build_program_in_subdir(
  r.flow
  DEPENDS
  grass_gis
  grass_raster
  grass_segment
  grass_vector
  grass_bitmap
  ${LIBM})

build_program_in_subdir(r.geomorphon DEPENDS grass_gis grass_raster grass_gmath
                        ${LIBM})

build_program_in_subdir(r.grow.distance DEPENDS grass_gis grass_raster ${LIBM})

build_program_in_subdir(r.gwflow DEPENDS grass_gis grass_raster grass_gpde
                        grass_gmath)

build_program_in_subdir(r.his DEPENDS grass_gis grass_raster grass_gproj)

build_program_in_subdir(
  r.horizon
  DEPENDS
  grass_gis
  grass_raster
  grass_gproj
  grass_parson
  ${LIBM})

build_program_in_subdir(r.in.ascii DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.in.bin DEPENDS grass_gis grass_raster ${LIBM})

build_program_in_subdir(
  r.in.gdal
  DEPENDS
  grass_gis
  grass_raster
  grass_gproj
  grass_imagery
  ${LIBM}
  PRIMARY_DEPENDS
  GDAL::GDAL)

build_program_in_subdir(r.in.gridatb DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.in.mat DEPENDS grass_gis grass_raster)

file(GLOB r_in_pdal_SOURCES "r.in.pdal/*.c" "r.in.pdal/*.cpp")
build_program_in_subdir(
  r.in.pdal
  SOURCES
  "${r_in_pdal_SOURCES}"
  DEPENDS
  grass_gis
  grass_raster
  grass_vector
  grass_gmath
  grass_segment
  grass_gproj
  ${LIBM}
  PRIMARY_DEPENDS
  ${PDAL})

build_program_in_subdir(
  r.in.png
  DEPENDS
  grass_gis
  grass_raster
  ${LIBM}
  PRIMARY_DEPENDS
  PNG::PNG)

build_program_in_subdir(r.in.poly DEPENDS grass_gis grass_raster)

if(NOT MSVC)
  build_program_in_subdir(r.in.xyz DEPENDS grass_gis grass_raster ${LIBM})
endif()
build_program_in_subdir(
  r.info
  TEST_SOURCES
  "test_r_info.py"
  DEPENDS
  grass_gis
  ${LIBM}
  grass_raster
  grass_parson)

build_program_in_subdir(r.kappa DEPENDS grass_gis grass_raster ${LIBM})

build_program_in_subdir(r.latlong DEPENDS grass_gis grass_raster grass_gproj)

build_program_in_subdir(r.lake DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.mask.status DEPENDS grass_gis grass_manage
                        grass_parson grass_raster)

build_program_in_subdir(
  r.mfilter
  DEPENDS
  grass_gis
  grass_raster
  grass_rowio
  OPTIONAL_DEPENDS
  OPENMP)

build_program_in_subdir(r.mode DEPENDS grass_gis grass_raster)

build_program_in_subdir(
  r.neighbors
  DEPENDS
  grass_gis
  grass_raster
  grass_stats
  ${LIBM}
  OPTIONAL_DEPENDS
  OPENMP)

build_program_in_subdir(r.null DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.object.geometry DEPENDS grass_gis grass_raster
                        grass_parson ${LIBM})

build_program_in_subdir(r.out.ascii DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.out.bin DEPENDS grass_gis grass_raster)

build_program_in_subdir(
  r.out.gdal
  DEPENDS
  grass_gis
  grass_gproj
  grass_imagery
  grass_gis
  grass_raster
  grass_dbmibase
  grass_imagery
  ${LIBM}
  PROJ::proj
  PRIMARY_DEPENDS
  GDAL::GDAL)

set_source_files_properties(
  r.out.gdal/main.c
  PROPERTIES COMPILE_DEFINITIONS
             "GRASS_VERSION_NUMBER=\"${GRASS_VERSION_NUMBER}\"")

build_program_in_subdir(r.out.gridatb DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.out.mat DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.out.mpeg DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.out.png DEPENDS grass_gis grass_raster
                        PRIMARY_DEPENDS PNG::PNG)

build_program_in_subdir(r.out.pov DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.out.ppm DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.out.ppm3 DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.out.vrml DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.out.vtk DEPENDS grass_gis grass_raster)

build_program_in_subdir(
  r.param.scale
  DEPENDS
  grass_gis
  grass_gmath
  grass_raster
  grass_interpfl
  grass_interpdata
  grass_qtree
  ${LIBM})

build_program_in_subdir(r.patch DEPENDS grass_gis grass_raster OPTIONAL_DEPENDS
                        OPENMP)

build_program_in_subdir(r.path DEPENDS grass_gis grass_raster grass_vector
                        ${LIBM})

build_program_in_subdir(r.profile DEPENDS grass_gis grass_raster grass_parson
                        ${LIBM})

build_program_in_subdir(
  r.proj
  DEPENDS
  grass_gis
  grass_raster
  grass_gproj
  ${LIBM}
  OPTIONAL_DEPENDS
  OPENMP)

build_program_in_subdir(r.quant DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.quantile DEPENDS grass_gis grass_raster ${LIBM})

build_program_in_subdir(
  r.random
  DEPENDS
  grass_gis
  grass_raster
  grass_vector
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  ${LIBM})

build_program_in_subdir(r.random.cells DEPENDS grass_gis grass_raster ${LIBM})

build_program_in_subdir(r.random.surface DEPENDS grass_gis grass_raster ${LIBM})

build_program_in_subdir(r.reclass DEPENDS grass_gis grass_raster grass_manage)

build_program_in_subdir(r.recode DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.region DEPENDS grass_gis grass_raster grass_vector)

build_program_in_subdir(r.regression.line DEPENDS grass_gis grass_raster
                        ${LIBM})

build_program_in_subdir(r.regression.multi DEPENDS grass_gis grass_raster
                        ${LIBM})

build_program_in_subdir(r.report DEPENDS grass_gis grass_raster grass_parson)

build_program_in_subdir(
  r.resamp.bspline
  DEPENDS
  grass_gis
  grass_gmath
  grass_raster
  grass_vector
  grass_segment
  grass_lidar
  ${LIBM})

build_program_in_subdir(
  r.resamp.filter
  DEPENDS
  grass_gis
  grass_raster
  ${LIBM}
  OPTIONAL_DEPENDS
  OPENMP)

build_program_in_subdir(
  r.resamp.interp
  DEPENDS
  grass_gis
  grass_raster
  ${LIBM}
  OPTIONAL_DEPENDS
  OPENMP)

build_program_in_subdir(
  r.resamp.rst
  DEPENDS
  grass_gis
  grass_raster
  grass_gmath
  grass_interpfl
  grass_interpdata
  grass_qtree)

build_program_in_subdir(r.resamp.stats DEPENDS grass_gis grass_raster
                        grass_stats ${LIBM})

build_program_in_subdir(r.resample DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.rescale DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.rescale.eq DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.ros DEPENDS grass_gis grass_raster ${LIBM})

build_program_in_subdir(
  r.series
  DEPENDS
  grass_gis
  grass_raster
  grass_stats
  OPTIONAL_DEPENDS
  OPENMP)

build_program_in_subdir(
  r.series.accumulate
  DEPENDS
  grass_gis
  grass_raster
  grass_stats
  OPTIONAL_DEPENDS
  OPENMP)

build_program_in_subdir(r.series.interp DEPENDS grass_gis grass_raster
                        grass_stats)

build_program_in_subdir(r.relief DEPENDS grass_gis grass_raster ${LIBM})

add_subdirectory(r.sim)

build_program_in_subdir(
  r.slope.aspect
  DEPENDS
  grass_gis
  grass_raster
  ${LIBM}
  OPTIONAL_DEPENDS
  OPENMP)

build_program_in_subdir(
  r.solute.transport
  DEPENDS
  grass_gis
  grass_raster
  grass_gmath
  grass_gpde
  ${LIBM})

add_subdirectory(r.spread)

build_program_in_subdir(r.spreadpath DEPENDS grass_gis grass_raster
                        grass_segment)

build_program_in_subdir(r.statistics DEPENDS grass_gis grass_raster ${LIBM})

build_program_in_subdir(r.stats.zonal DEPENDS grass_gis grass_raster ${LIBM})

build_program_in_subdir(r.stats.quantile DEPENDS grass_gis grass_raster ${LIBM})

build_program_in_subdir(r.stats DEPENDS grass_gis grass_raster ${LIBM})

build_program_in_subdir(
  r.stream.extract
  DEPENDS
  grass_gis
  grass_raster
  grass_segment
  grass_vector
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  ${LIBM})

build_program_in_subdir(
  r.sun
  DEPENDS
  grass_gis
  grass_gmath
  grass_raster
  grass_gproj
  ${LIBM}
  OPTIONAL_DEPENDS
  OPENMP)

build_program_in_subdir(r.sunhours DEPENDS grass_gis grass_raster grass_gproj
                        ${LIBM})

build_program_in_subdir(r.sunmask DEPENDS grass_gis grass_raster grass_gproj
                        ${LIBM})

build_program_in_subdir(r.support DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.support.stats DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.surf.area DEPENDS grass_gis grass_raster ${LIBM})

build_program_in_subdir(r.surf.contour DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.surf.fractal DEPENDS grass_gis grass_raster
                        grass_gmath ${LIBM})

build_program_in_subdir(r.surf.gauss DEPENDS grass_gis grass_raster grass_gmath)

build_program_in_subdir(r.surf.idw DEPENDS grass_gis grass_raster ${LIBM})

build_program_in_subdir(r.surf.random DEPENDS grass_gis grass_raster
                        grass_gmath)

if(MSVC)
  set(USER_NAME $ENV{USERNAME})
else()
  set(USER_NAME $ENV{USER})
endif()

build_program_in_subdir(r.texture DEPENDS grass_gis grass_raster ${LIBM})

build_program_in_subdir(r.thin DEPENDS grass_gis grass_raster grass_rowio)

build_program_in_subdir(r.tile DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.timestamp DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.to.rast3 DEPENDS grass_gis grass_raster
                        grass_raster3d)

build_program_in_subdir(r.to.rast3elev DEPENDS grass_gis grass_raster
                        grass_raster3d)

build_program_in_subdir(
  r.to.vect
  DEPENDS
  grass_gis
  grass_raster
  grass_vector
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver)

build_program_in_subdir(r.topidx DEPENDS grass_gis grass_raster ${LIBM})

build_program_in_subdir(r.topmodel DEPENDS grass_gis grass_raster ${LIBM})

build_program_in_subdir(r.transect DEPENDS grass_gis grass_raster ${LIBM})

add_subdirectory(r.univar)

build_program_in_subdir(r.uslek DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.usler DEPENDS grass_gis grass_raster)

build_program_in_subdir(
  r.volume
  DEPENDS
  grass_gis
  grass_raster
  grass_vector
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver)

build_program_in_subdir(
  r.walk
  DEPENDS
  grass_gis
  grass_raster
  grass_segment
  grass_vector
  ${LIBM})

build_program_in_subdir(r.water.outlet DEPENDS grass_gis grass_raster)

build_program_in_subdir(r.what DEPENDS grass_gis grass_raster grass_vector
                        grass_parson)

build_program_in_subdir(r.what.color DEPENDS grass_gis grass_raster)

build_program_in_subdir(
  r.in.lidar
  DEPENDS
  grass_gis
  grass_vector
  grass_raster
  grass_lidar
  grass_gproj
  grass_segment
  PRIMARY_DEPENDS
  LIBLAS
  ${LIBM})

add_subdirectory(r.li)

add_subdirectory(r.mapcalc)

build_program_in_subdir(
  r.viewshed
  DEPENDS
  grass_gis
  grass_raster
  grass_iostream
  ${LIBM}
  SRC_REGEX
  "*.cpp"
  DEFS
  "-DUSER=${USER_NAME}")

build_program_in_subdir(
  r.terraflow
  DEPENDS
  grass_gis
  grass_raster
  grass_iostream
  ${LIBM}
  SRC_REGEX
  "*.cpp"
  DEFS
  "-DUSER=${USER_NAME};-DNODATA_FIX;-DELEV_FLOAT")

add_subdirectory(r.watershed)
