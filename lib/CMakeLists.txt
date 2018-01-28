
#order is relevant:


build_library_in_subdir(datetime)

set(DL_LIBRARY)
add_subdirectory(gis)
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  find_library(DL_LIBRARY dl)
endif()

build_library_in_subdir(raster
  DEFS "-DGDAL_DYNAMIC=1;-DGDAL_LINK=1"
  DEPENDS GDAL PROJ4 ${DL_LIBRARY}
  )

configure_file(external/ccmath/ccmath.h
  ${CMAKE_BINARY_DIR}/include/grass/ccmath_grass.h
  COPYONLY)
	     
build_library_in_subdir(external/ccmath)

build_library_in_subdir(external/shapelib NAME grass_shape
  HEADERS "shapefil.h")

build_library_in_subdir(gmath
  DEPENDS grass_ccmath
  OPTIONAL_DEPENDS FFTW )

build_library_in_subdir(linkm)

build_library_in_subdir(driver DEPENDS FREETYPE)

build_library_in_subdir(pngdriver
  DEPENDS grass_driver LIBPNG ZLIB
  INCLUDES "./driver"
  )
build_library_in_subdir(psdriver
  DEPENDS grass_driver
  )
build_library_in_subdir(htmldriver
  DEPENDS grass_driver
  )

set(_grass_display_DEFS)

set(_grass_display_DEPENDS
  grass_driver grass_raster
  grass_htmldriver grass_pngdriver grass_psdriver) 

if(WITH_CAIRO)
  
  set(_cairodriver_DEPENDS CAIRO FREETYPE grass_driver)
  if(WITH_X11)
    list(APPEND _cairodriver_DEPENDS X11)
  endif()

  build_library_in_subdir(cairodriver
    DEPENDS ${_cairodriver_DEPENDS}
    )

  set(_grass_display_DEFS "-DUSE_CAIRO")
  list(APPEND _grass_display_DEPENDS grass_cairodriver)
endif()


build_library_in_subdir(bitmap SOURCES "./bitmap/bitmap.c" "./bitmap/sparse.c")

build_library_in_subdir(btree)

build_library_in_subdir(btree2 HEADERS "kdtree.h")

build_library_in_subdir(display
  DEFS ${_grass_display_DEFS}
  DEPENDS ${_grass_display_DEPENDS}
  )

add_subdirectory(db)

add_subdirectory(fonts)

add_subdirectory(proj)

add_subdirectory(vector)

build_library_in_subdir(imagery DEPENDS GDAL GEOS grass_vector)

build_library_in_subdir(cluster)

build_library_in_subdir(rowio)

build_library_in_subdir(segment)

add_subdirectory(rst)

build_library_in_subdir(lidar DEPENDS GDAL GEOS grass_segment HEADERS "lidar.h")

build_library_in_subdir(raster3d DEPENDS grass_raster)

build_program_in_subdir(raster3d/test
  NAME test.raster3d.lib
  DEPENDS grass_gis grass_raster3d 
  )

build_library_in_subdir(gpde HEADERS "N_*.h"
  DEPENDS grass_gis grass_raster3d grass_gmath
  )

build_library_in_subdir(dspf)

build_library_in_subdir(symbol NAME grass_sym)

add_subdirectory(init)

build_library_in_subdir(cdhc)

build_library_in_subdir(stats)

build_library_in_subdir(arraystats)

if(WITH_OPENGL)
 # set(ogsf_DEPENDS OPENGL GDAL TIFF)
 # build_library_in_subdir(ogsf DEPENDS ${ogsf_DEPENDS})
 build_library_in_subdir(ogsf DEPENDS OPENGL GDAL TIFF)
 build_library_in_subdir(nviz DEPENDS OPENGL GDAL TIFF)
endif()


add_subdirectory(temporal)

#add_subdirectory(python)

build_library_in_subdir(iostream SRC_REGEX "*.cpp")

build_library_in_subdir(manage DEPENDS grass_gis grass_vector grass_raster3d )

build_library_in_subdir(calc)
