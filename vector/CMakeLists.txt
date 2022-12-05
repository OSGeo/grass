
build_program_in_subdir(v.buffer DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.build DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.build.polylines DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.category DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.class DEPENDS grass_gis grass_vector grass_arraystats)

build_program_in_subdir(v.clean DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.cluster DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.colors DEPENDS grass_gis grass_vector grass_raster3d)

build_program_in_subdir(v.colors.out DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.db.connect DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.db.select DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.decimate DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.delaunay DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.distance DEPENDS grass_gis grass_vector grass_gmath)

build_program_in_subdir(v.drape DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.edit DEPENDS grass_gis grass_vector grass_vedit)

build_program_in_subdir(v.extract DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.extrude DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.generalize DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.hull DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.info DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.in.ascii DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.in.db DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.in.dxf DEPENDS grass_gis grass_vector)

file(GLOB v_in_pdal_SOURCES "v.in.pdal/*.c" "v.in.pdal/main.cpp")
build_program_in_subdir(
  v.in.pdal
  SOURCES "${v_in_pdal_SOURCES}"
  DEPENDS grass_gis grass_vector
  PRIMARY_DEPENDS PDAL)

build_program_in_subdir(v.in.region DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.kcv DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.kernel DEPENDS grass_gis grass_vector )

build_program_in_subdir(v.label DEPENDS grass_gis grass_vector grass_display)

build_program_in_subdir(v.label.sa DEPENDS grass_gis grass_vector grass_display)

build_program_in_subdir(v.proj DEPENDS grass_gis grass_vector grass_gproj)

build_program_in_subdir(v.profile DEPENDS grass_gis grass_vector)

add_subdirectory(v.lrs)

build_program_in_subdir(v.mkgrid DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.neighbors DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.net DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.net.alloc DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.net.allpairs DEPENDS grass_gis grass_vector grass_neta)

build_program_in_subdir(v.net.bridge DEPENDS grass_gis grass_vector grass_neta)

build_program_in_subdir(v.net.centrality DEPENDS grass_gis grass_vector grass_neta)

build_program_in_subdir(v.net.components DEPENDS grass_gis grass_vector grass_neta)

build_program_in_subdir(v.net.connectivity DEPENDS grass_gis grass_vector grass_neta)

build_program_in_subdir(v.net.distance DEPENDS grass_gis grass_vector grass_neta)

build_program_in_subdir(v.net.flow DEPENDS grass_gis grass_vector grass_neta)

build_program_in_subdir(v.net.iso DEPENDS grass_gis grass_vector grass_neta)

build_program_in_subdir(v.net.path DEPENDS grass_gis grass_vector grass_neta)

build_program_in_subdir(v.net.salesman DEPENDS grass_gis grass_vector grass_neta)

build_program_in_subdir(v.net.spanningtree DEPENDS grass_gis grass_vector grass_neta)

build_program_in_subdir(v.net.steiner DEPENDS grass_gis grass_vector grass_neta)

build_program_in_subdir(v.net.timetable DEPENDS grass_gis grass_vector grass_neta)

build_program_in_subdir(v.net.visibility DEPENDS grass_gis grass_vector grass_neta)

build_program_in_subdir(v.normal DEPENDS grass_gis grass_vector grass_cdhc)

build_program_in_subdir(v.out.ascii DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.out.dxf DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.out.pov DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.out.svg DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.out.vtk DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.outlier DEPENDS grass_gis grass_vector grass_lidar)

build_program_in_subdir(v.overlay DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.parallel DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.patch DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.perturb DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.split DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.qcount DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.random DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.reclass DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.rectify DEPENDS grass_gis grass_vector grass_imagery)

build_program_in_subdir(v.sample DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.segment DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.select DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.support DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.surf.bspline DEPENDS grass_gis grass_vector grass_lidar grass_gpde)

build_program_in_subdir(v.surf.idw DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.surf.rst
	DEPENDS grass_gis grass_vector grass_interpdata grass_interpfl grass_qtree)

build_program_in_subdir(v.transform DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.timestamp DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.to.3d DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.to.db DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.to.points DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.to.rast DEPENDS grass_gis grass_vector grass_raster)

build_program_in_subdir(v.to.rast3 DEPENDS grass_gis grass_vector grass_raster3d)

build_program_in_subdir(v.type DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.univar DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.voronoi DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.what DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.what.rast DEPENDS grass_gis grass_vector grass_raster)

build_program_in_subdir(v.what.rast3 DEPENDS grass_gis grass_vector grass_raster3d)

build_program_in_subdir(v.vect.stats DEPENDS grass_gis grass_vector grass_stats)

build_program_in_subdir(v.vol.rst
	DEPENDS grass_gis grass_vector grass_raster3d grass_bitmap)

build_program_in_subdir(v.out.ogr DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.in.ogr DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.external DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.external.out DEPENDS grass_gis grass_vector)

build_program_in_subdir(v.in.lidar DEPENDS grass_gis grass_vector  PRIMARY_DEPENDS LIBLAS)

build_program_in_subdir(v.lidar.correction DEPENDS grass_gis grass_vector grass_lidar
   PRIMARY_DEPENDS LIBLAS)

build_program_in_subdir(v.lidar.edgedetection
   DEPENDS grass_gis grass_vector grass_lidar grass_dbmibase grass_gmath
   PRIMARY_DEPENDS LIBLAS)

build_program_in_subdir(v.lidar.growing DEPENDS grass_gis grass_vector PRIMARY_DEPENDS LIBLAS)

build_program_in_subdir(v.out.lidar DEPENDS grass_gis grass_vector PRIMARY_DEPENDS LIBLAS)

build_program_in_subdir(v.out.postgis DEPENDS grass_gis grass_vector PRIMARY_DEPENDS POSTGRES)

if(WITH_OPENDWG)
	build_program_in_subdir(v.in.dwg DEPENDS grass_gis grass_vector)
endif()
