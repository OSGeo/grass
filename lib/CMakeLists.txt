
set(use_math_DEFS "")
if(MSVC)
  set(use_math_DEFS "-D_USE_MATH_DEFINES=1")
endif()

build_library_in_subdir(datetime)

add_subdirectory(gis)

build_library_in_subdir(driver
  DEFS "${use_math_DEFS}"
  DEPENDS grass_gis FREETYPE ICONV ZLIB)

add_subdirectory(proj)

file(GLOB raster_SRCS "./raster/*.c")
list(APPEND raster_SRCS "./gis/gisinit.c")
build_library_in_subdir(raster
  DEFS "-DGDAL_DYNAMIC=1;-DGDAL_LINK=1"
  SOURCES ${raster_SRCS}
  DEPENDS GDAL PROJ grass_gproj)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  find_library(DL_LIBRARY dl)
  mark_as_advanced(DL_LIBRARY)
  target_link_libraries(grass_raster ${DL_LIBRARY})
endif()

configure_file(external/ccmath/ccmath.h
  ${CMAKE_BINARY_DIR}/include/grass/ccmath_grass.h
  COPYONLY)
build_library_in_subdir(external/ccmath)

build_library_in_subdir(external/shapelib NAME grass_shape
  HEADERS "shapefil.h")

build_library_in_subdir(gmath
  DEPENDS grass_ccmath grass_gis
  DEFS "${use_math_DEFS}"
  OPTIONAL_DEPENDS FFTW)

build_library_in_subdir(linkm)

file(GLOB pngdriver_SRCS "./pngdriver/*.c")
list(APPEND pngdriver_SRCS "./driver/init.c")
build_library_in_subdir(pngdriver
  SOURCES ${pngdriver_SRCS}
  DEPENDS grass_driver LIBPNG ZLIB
  INCLUDES "./driver")

file(GLOB psdriver_SRCS "./psdriver/*.c")
list(APPEND psdriver_SRCS "./driver/init.c")
build_library_in_subdir(psdriver
  SOURCES ${psdriver_SRCS}
  DEPENDS grass_driver
  INCLUDES "./driver")

file(GLOB htmldriver_SRCS "./htmldriver/*.c")
list(APPEND htmldriver_SRCS "./driver/init.c")
build_library_in_subdir(htmldriver
  SOURCES ${htmldriver_SRCS}
  DEPENDS grass_driver
  INCLUDES "./driver")

set(_grass_display_DEFS)

set(_grass_display_DEPENDS
  grass_driver grass_raster
  grass_htmldriver grass_pngdriver grass_psdriver)

if(WITH_CAIRO)

  set(_cairodriver_DEPENDS CAIRO FREETYPE grass_driver)
  if(WITH_X11)
    list(APPEND _cairodriver_DEPENDS X11)
  endif()

  file(GLOB cairodriver_SRCS "./cairodriver/*.c")
  list(APPEND cairodriver_SRCS "./driver/init.c")
  build_library_in_subdir(cairodriver
    DEPENDS ${_cairodriver_DEPENDS}
	SOURCES ${cairodriver_SRCS})

  set(_grass_display_DEFS "-DUSE_CAIRO")
  list(APPEND _grass_display_DEPENDS grass_cairodriver)
endif()

build_library_in_subdir(bitmap
SOURCES "./bitmap/bitmap.c" "./bitmap/sparse.c"
DEPENDS grass_linkm)

build_library_in_subdir(btree)

build_library_in_subdir(btree2 HEADERS "kdtree.h" DEPENDS grass_gis)

build_library_in_subdir(display
  DEFS ${_grass_display_DEFS}
  DEPENDS ${_grass_display_DEPENDS})

add_subdirectory(db)

add_subdirectory(fonts)

add_subdirectory(vector)

build_library_in_subdir(imagery DEPENDS GDAL GEOS grass_vector)

build_library_in_subdir(cluster DEPENDS grass_imagery)

build_library_in_subdir(rowio DEPENDS grass_gis)

build_library_in_subdir(segment DEPENDS grass_gis) #addeed DEPENDS grass_gis for uninstd.h

add_subdirectory(rst)

build_library_in_subdir(lidar
DEPENDS GDAL GEOS grass_vector grass_raster grass_dbmibase grass_segment
HEADERS "lidar.h")

build_library_in_subdir(raster3d DEPENDS grass_raster)

build_program_in_subdir(raster3d/test
  NAME test.raster3d.lib
  DEPENDS grass_gis grass_raster3d grass_raster)

build_library_in_subdir(gpde HEADERS "N_*.h"
  DEPENDS grass_gis grass_raster3d grass_gmath)

build_library_in_subdir(dspf  DEPENDS grass_gis)

build_library_in_subdir(symbol NAME grass_sym DEPENDS grass_raster)

add_subdirectory(init)

build_library_in_subdir(cdhc DEPENDS grass_raster)

build_library_in_subdir(stats DEPENDS grass_raster)

build_library_in_subdir(arraystats DEPENDS grass_gis)

if(WITH_OPENGL)
	build_library_in_subdir(ogsf
	DEPENDS grass_raster grass_raster3d grass_vector grass_bitmap OPENGL TIFF)

	build_library_in_subdir(nviz
	DEPENDS grass_display grass_raster grass_vector grass_bitmap grass_ogsf OPENGL TIFF)
endif()

add_subdirectory(temporal)

build_library_in_subdir(iostream SRC_REGEX "*.cpp" DEPENDS grass_gis)

build_library_in_subdir(manage DEPENDS grass_raster grass_vector grass_raster3d )
file(COPY manage/element_list DESTINATION ${GISBASE}/etc)
install(FILES ${GISBASE}/etc/element_list DESTINATION etc)

build_library_in_subdir(calc
DEPENDS grass_raster)
