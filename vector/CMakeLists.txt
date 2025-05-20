set(vector_modules_list
    v.build
    v.build.polylines
    v.category
    v.class
    v.clean
    v.cluster
    v.colors
    v.colors.out
    v.db.connect
    v.db.select
    v.decimate
    v.delaunay
    v.distance
    v.drape
    v.edit
    v.external
    v.external.out
    v.extract
    v.extrude
    v.fill.holes
    v.generalize
    v.hull
    v.in.ascii
    v.in.db
    v.in.dxf
    v.in.lidar
    v.in.ogr
    v.in.pdal
    v.in.region
    v.info
    v.kcv
    v.kernel
    v.label
    v.label.sa
    v.lidar.correction
    v.lidar.edgedetection
    v.lidar.growing
    v.lrs.create
    v.lrs.label
    v.lrs.segment
    v.lrs.where
    v.mkgrid
    v.neighbors
    v.net
    v.net.alloc
    v.net.allpairs
    v.net.bridge
    v.net.centrality
    v.net.components
    v.net.connectivity
    v.net.distance
    v.net.flow
    v.net.iso
    v.net.path
    v.net.salesman
    v.net.spanningtree
    v.net.steiner
    v.net.timetable
    v.net.visibility
    v.normal
    v.out.ascii
    v.out.dxf
    v.out.lidar
    v.out.ogr
    v.out.postgis
    v.out.pov
    v.out.svg
    v.out.vtk
    v.outlier
    v.overlay
    v.parallel
    v.patch
    v.perturb
    v.profile
    v.proj
    v.qcount
    v.random
    v.reclass
    v.rectify
    v.sample
    v.segment
    v.select
    v.split
    v.support
    v.surf.bspline
    v.surf.idw
    v.surf.rst
    v.timestamp
    v.to.3d
    v.to.db
    v.to.points
    v.to.rast
    v.to.rast3
    v.transform
    v.type
    v.univar
    v.vect.stats
    v.vol.rst
    v.voronoi
    v.what
    v.what.rast
    v.what.rast3)

if(WITH_GEOS)
  list(APPEND vector_modules_list v.buffer)
endif()

add_custom_target(
  ALL_VECTOR_MODULES ALL
  DEPENDS ${vector_modules_list}
  COMMENT "All vector modules are built.")

if(WITH_DOCS)
  generate_html(TARGET ALL_VECTOR_MODULES NAME vectorintro)
  generate_html(TARGET ALL_VECTOR_MODULES NAME lrs SOURCEDIR
                ${CMAKE_CURRENT_SOURCE_DIR}/v.lrs)
endif()

# build v.buffer only with GEOS following
# e5ae98f9a02d81f89be5776c8be5841f1d259b7f
build_program_in_subdir(
  v.buffer
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector
  ${LIBM}
  PRIMARY_DEPENDS
  GEOS::geos_c)

build_program_in_subdir(v.build DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.build.polylines DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.category DEPENDS grass_gis grass_vector grass_parson)

build_program_in_subdir(
  v.class
  DEPENDS
  grass_arraystats
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector)

build_program_in_subdir(v.clean DEPENDS grass_gis grass_vector ${LIBM})

build_program_in_subdir(v.cluster DEPENDS grass_btree2 grass_gis grass_vector
                        ${LIBM})

build_program_in_subdir(
  v.colors
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_raster
  grass_raster3d
  grass_vector)

build_program_in_subdir(
  v.colors.out
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_raster
  grass_vector)

build_program_in_subdir(
  v.db.connect
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector)

build_program_in_subdir(
  v.db.select
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector)

build_program_in_subdir(
  v.decimate
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_raster
  grass_vector)

build_program_in_subdir(v.delaunay DEPENDS grass_gis grass_vector)

build_program_in_subdir(
  v.distance
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_gmath
  grass_vector
  grass_parson
  ${LIBM})

build_program_in_subdir(v.drape DEPENDS grass_gis grass_raster grass_vector)

build_program_in_subdir(
  v.edit
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector
  grass_vedit)

build_program_in_subdir(
  v.extract
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_gmath
  grass_vector
  ${LIBM})

build_program_in_subdir(
  v.extrude
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_raster
  grass_vector)

build_program_in_subdir(
  v.fill.holes
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector)

build_program_in_subdir(
  v.generalize
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_dgl
  grass_dig2
  grass_gis
  grass_vector
  ${LIBM})

build_program_in_subdir(v.hull DEPENDS grass_gis grass_vector)

build_program_in_subdir(
  v.info
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_dig2
  grass_gis
  grass_vector
  grass_parson)

build_program_in_subdir(
  v.in.ascii
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector)

build_program_in_subdir(
  v.in.db
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector
  ${LIBM})

build_program_in_subdir(
  v.in.dxf
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector
  ${LIBM})

file(GLOB v_in_pdal_SOURCES "v.in.pdal/*.c" "v.in.pdal/main.cpp")
build_program_in_subdir(
  v.in.pdal
  SOURCES
  "${v_in_pdal_SOURCES}"
  DEPENDS
  grass_gis
  grass_gproj
  grass_vector
  ${LIBM}
  PRIMARY_DEPENDS
  ${PDAL})

build_program_in_subdir(
  v.in.region
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_raster
  grass_vector)

build_program_in_subdir(
  v.kcv
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector
  ${LIBM})

build_program_in_subdir(
  v.kernel
  DEPENDS
  grass_gis
  grass_gmath
  grass_raster
  grass_vector
  ${LIBM})

build_program_in_subdir(
  v.label
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_display
  grass_gis
  grass_vector)

build_program_in_subdir(
  v.label.sa
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_dig2
  grass_display
  grass_gis
  grass_vector
  ${LIBM}
  PRIMARY_DEPENDS
  Freetype::Freetype)

build_program_in_subdir(v.proj DEPENDS grass_gis grass_gproj grass_vector
                        ${LIBM})

build_program_in_subdir(
  v.profile
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_gis
  grass_vector
  OPTIONAL_DEPENDS
  GEOS::geos_c)

add_subdirectory(v.lrs)

build_program_in_subdir(
  v.mkgrid
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector
  ${LIBM})

build_program_in_subdir(
  v.neighbors
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_raster
  grass_stats
  grass_vector
  ${LIBM})

build_program_in_subdir(
  v.net
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector
  ${LIBM})

build_program_in_subdir(
  v.net.alloc
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_dgl
  grass_gis
  grass_vector)

build_program_in_subdir(
  v.net.allpairs
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_neta
  grass_vector)

build_program_in_subdir(v.net.bridge DEPENDS grass_gis grass_neta grass_vector)

build_program_in_subdir(
  v.net.centrality
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_dgl
  grass_gis
  grass_neta
  grass_vector)

build_program_in_subdir(
  v.net.components
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_neta
  grass_vector)

build_program_in_subdir(v.net.connectivity DEPENDS grass_gis grass_neta
                        grass_vector)

build_program_in_subdir(
  v.net.distance
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_dgl
  grass_gis
  grass_neta
  grass_vector)

build_program_in_subdir(
  v.net.flow
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_neta
  grass_vector)

build_program_in_subdir(
  v.net.iso
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_dgl
  grass_gis
  grass_vector)

build_program_in_subdir(
  v.net.path
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector)

build_program_in_subdir(v.net.salesman DEPENDS grass_gis grass_neta
                        grass_vector)

build_program_in_subdir(v.net.spanningtree DEPENDS grass_gis grass_neta
                        grass_vector)

build_program_in_subdir(v.net.steiner DEPENDS grass_gis grass_neta grass_vector)

build_program_in_subdir(
  v.net.timetable
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_neta
  grass_vector)

build_program_in_subdir(v.net.visibility DEPENDS grass_dgl grass_gis
                        grass_vector)

build_program_in_subdir(
  v.normal
  DEPENDS
  grass_cdhc
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector
  ${LIBM})

build_program_in_subdir(v.out.ascii DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.out.dxf DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.out.pov DEPENDS grass_gis grass_vector)

build_program_in_subdir(
  v.out.svg
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector)

build_program_in_subdir(
  v.out.vtk
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector)

build_program_in_subdir(
  v.outlier
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_gmath
  grass_lidar
  grass_vector
  ${LIBM})

build_program_in_subdir(
  v.overlay
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector)

build_program_in_subdir(v.parallel DEPENDS grass_gis grass_vector)

build_program_in_subdir(
  v.patch
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector
  ${LIBM})

build_program_in_subdir(
  v.perturb
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector
  ${LIBM})

build_program_in_subdir(v.split DEPENDS grass_gis grass_vector ${LIBM})

build_program_in_subdir(v.qcount DEPENDS grass_gis grass_vector ${LIBM})

build_program_in_subdir(
  v.random
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector)

build_program_in_subdir(
  v.reclass
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector)

build_program_in_subdir(
  v.rectify
  DEPENDS
  grass_gis
  grass_gmath
  grass_imagery
  grass_vector
  ${LIBM})

build_program_in_subdir(
  v.sample
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_raster
  grass_vector)

build_program_in_subdir(v.segment DEPENDS grass_gis grass_vector ${LIBM})

build_program_in_subdir(
  v.select
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector
  OPTIONAL_DEPENDS
  GEOS::geos_c)

build_program_in_subdir(v.support DEPENDS grass_gis grass_vector)

build_program_in_subdir(
  v.surf.bspline
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_gmath
  grass_gpde
  grass_lidar
  grass_raster
  grass_segment
  grass_vector
  ${LIBM})

build_program_in_subdir(
  v.surf.idw
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_raster
  grass_vector
  ${LIBM})

build_program_in_subdir(
  v.surf.rst
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_gmath
  grass_interpdata
  grass_interpfl
  grass_qtree
  grass_vector
  grass_raster
  ${LIBM}
  OPTIONAL_DEPENDS
  OPENMP)

build_program_in_subdir(
  v.transform
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector
  ${LIBM})

build_program_in_subdir(v.timestamp DEPENDS grass_gis grass_vector)

build_program_in_subdir(
  v.to.3d
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector)

build_program_in_subdir(
  v.to.db
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector
  ${LIBM})

build_program_in_subdir(
  v.to.points
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector
  ${LIBM})

build_program_in_subdir(
  v.to.rast
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_raster
  grass_vector
  ${LIBM})

build_program_in_subdir(
  v.to.rast3
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_raster3d
  grass_vector
  ${LIBM})

build_program_in_subdir(v.type DEPENDS grass_gis grass_vector)

# XXX: to workaround an MSVC bug where NAN is not a constant
# v.univar\main.c(77,8): error C2099: initializer is not a constant
# REMOVEME later once the fix is released
# https://developercommunity.visualstudio.com/t/NAN-is-no-longer-compile-time-constant-i/10688907#T-N10849410
set(ucrt_noisy_nan_DEFS "")
if(WIN32)
  set(ucrt_noisy_nan_DEFS "-D_UCRT_NOISY_NAN")
endif()

build_program_in_subdir(
  v.univar
  DEFS
  "${ucrt_noisy_nan_DEFS}"
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector
  grass_parson
  ${LIBM})

build_program_in_subdir(
  v.voronoi
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_dig2
  grass_gis
  grass_vector
  ${LIBM})

build_program_in_subdir(
  v.what
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector)

build_program_in_subdir(
  v.what.rast
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_raster
  grass_vector)

build_program_in_subdir(
  v.what.rast3
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_raster3d
  grass_vector)

build_program_in_subdir(
  v.vect.stats
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_stats
  grass_vector)

build_program_in_subdir(
  v.vol.rst
  DEPENDS
  grass_bitmap
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_raster
  grass_raster3d
  grass_vector
  ${LIBM})

build_program_in_subdir(
  v.out.ogr
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_gproj
  grass_vector
  PRIMARY_DEPENDS
  PROJ::proj
  GDAL::GDAL)

build_program_in_subdir(
  v.in.ogr
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_gproj
  grass_vector
  ${LIBM}
  PRIMARY_DEPENDS
  GDAL::GDAL)

build_program_in_subdir(
  v.external
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_gproj
  grass_vector
  PROJ::proj
  PRIMARY_DEPENDS
  GDAL::GDAL
  OPTIONAL_DEPENDS
  PostgreSQL::PostgreSQL)

build_program_in_subdir(v.external.out DEPENDS grass_gis grass_vector
                        PRIMARY_DEPENDS GDAL::GDAL)

build_program_in_subdir(
  v.in.lidar
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_gproj
  grass_vector
  ${LIBM}
  PRIMARY_DEPENDS
  LIBLAS)

build_program_in_subdir(
  v.lidar.correction
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_gmath
  grass_lidar
  grass_vector
  ${LIBM}
  PRIMARY_DEPENDS
  LIBLAS)

build_program_in_subdir(
  v.lidar.edgedetection
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_gmath
  grass_lidar
  grass_vector
  ${LIBM}
  PRIMARY_DEPENDS
  LIBLAS)

build_program_in_subdir(
  v.lidar.growing
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_gmath
  grass_raster
  grass_vector
  PRIMARY_DEPENDS
  LIBLAS)

build_program_in_subdir(
  v.out.lidar
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_gproj
  grass_raster
  grass_vector
  ${LIBM}
  PRIMARY_DEPENDS
  LIBLAS)

build_program_in_subdir(
  v.out.postgis
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gis
  grass_vector
  PRIMARY_DEPENDS
  PostgreSQL::PostgreSQL)
