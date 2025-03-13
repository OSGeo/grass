set(use_math_DEFS "")
if(MSVC)
  set(use_math_DEFS "-D_USE_MATH_DEFINES=1")
endif()

build_library_in_subdir(datetime)

add_subdirectory(gis)

build_library_in_subdir(
  driver
  DEFS
  "${use_math_DEFS}"
  DEPENDS
  grass_gis
  OPTIONAL_DEPENDS
  Freetype::Freetype
  Iconv::Iconv)

add_subdirectory(proj)

build_library_in_subdir(external/parson NAME grass_parson HEADERS "gjson.h" "parson.h")
build_program_in_subdir(external/parson/test NAME test.gjson.lib DEPENDS grass_gis grass_parson)

build_library_in_subdir(
  raster
  DEFS
  "-DGDAL_LINK=1"
  DEPENDS
  GDAL::GDAL
  PROJ::proj
  grass_gis
  grass_gproj
  grass_parson)

if(TARGET LAPACKE)
  target_link_libraries(grass_raster PRIVATE LAPACKE)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  find_library(DL_LIBRARY dl)
  mark_as_advanced(DL_LIBRARY)
  target_link_libraries(grass_raster PRIVATE ${DL_LIBRARY})
endif()

configure_file(
  external/ccmath/ccmath.h
  ${OUTDIR}/${GRASS_INSTALL_INCLUDEDIR}/grass/ccmath_grass.h COPYONLY)
build_library_in_subdir(external/ccmath)

build_library_in_subdir(external/shapelib NAME grass_shape HEADERS "shapefil.h")

build_library_in_subdir(
  gmath
  DEPENDS
  grass_ccmath
  grass_gis
  DEFS
  "${use_math_DEFS}"
  OPTIONAL_DEPENDS
  CBLAS::CBLAS
  FFTW
  LAPACKE::LAPACKE
  OPENMP)

build_library_in_subdir(linkm)

file(GLOB pngdriver_SRCS "./pngdriver/*.c")
if(NOT WITH_LIBPNG)
  list(FILTER pngdriver_SRCS EXCLUDE REGEX [[./pngdriver/.*_png\.c$]])
endif()
build_library_in_subdir(
  pngdriver
  SOURCES
  ${pngdriver_SRCS}
  DEPENDS
  grass_driver
  grass_gis
  OPTIONAL_DEPENDS
  PNG::PNG)

build_library_in_subdir(
  psdriver
  DEPENDS
  grass_driver
  grass_gis
  INCLUDES
  "./driver")
add_custom_command(
  TARGET grass_psdriver
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/psdriver/psdriver.ps
    ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/psdriver.ps)
install(FILES ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/psdriver.ps
        DESTINATION ${GRASS_INSTALL_ETCDIR})

build_library_in_subdir(
  htmldriver
  DEPENDS
  grass_gis
  grass_driver
  INCLUDES
  "./driver")

set(_grass_display_DEFS)

set(_grass_display_DEPENDS grass_driver grass_raster grass_htmldriver
                           grass_pngdriver grass_psdriver grass_gis)

if(WITH_CAIRO)
  set(_cairodriver_DEPENDS
      Cairo::Cairo
      Fontconfig::Fontconfig
      Freetype::Freetype
      grass_gis
      grass_driver
      OPTIONAL_DEPENDS
      Iconv::Iconv)
  if(WITH_X11)
    list(PREPEND _cairodriver_DEPENDS X11::X11)
  endif()
  build_library_in_subdir(cairodriver DEPENDS ${_cairodriver_DEPENDS})

  set(_grass_display_DEFS "-DUSE_CAIRO")
  list(APPEND _grass_display_DEPENDS grass_cairodriver)
endif()

build_library_in_subdir(bitmap SOURCES "./bitmap/bitmap.c" "./bitmap/sparse.c"
                        DEPENDS grass_linkm)

build_library_in_subdir(btree)

build_library_in_subdir(btree2 HEADERS "kdtree.h" DEPENDS grass_gis)

build_library_in_subdir(display DEFS ${_grass_display_DEFS} DEPENDS
                        ${_grass_display_DEPENDS})

add_subdirectory(db)

add_subdirectory(fonts)

add_subdirectory(vector)

build_library_in_subdir(imagery DEPENDS grass_gis grass_vector grass_raster GDAL::GDAL)

build_library_in_subdir(cluster DEPENDS grass_imagery grass_gis grass_raster)

build_library_in_subdir(rowio DEPENDS grass_gis)

build_library_in_subdir(segment DEPENDS grass_gis) # addeed DEPENDS grass_gis
                                                   # for uninstd.h

add_subdirectory(rst)

build_library_in_subdir(
  lidar
  DEPENDS
  grass_gis
  grass_vector
  grass_raster
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_gmath
  grass_segment
  GDAL::GDAL
  HEADERS
  "lidar.h")

build_library_in_subdir(raster3d NAME grass_raster3d DEPENDS grass_raster
                        grass_gis)

build_program_in_subdir(
  raster3d/test
  NAME
  test.raster3d.lib
  DEPENDS
  grass_gis
  grass_interpfl
  grass_raster
  grass_raster3d
  GDAL::GDAL)

build_library_in_subdir(
  gpde
  HEADERS
  "N_*.h"
  DEPENDS
  grass_gis
  grass_raster
  grass_raster3d
  grass_gmath
  ${LIBM}
  OPTIONAL_DEPENDS
  OPENMP)

build_library_in_subdir(dspf DEPENDS grass_gis)

build_library_in_subdir(symbol NAME grass_symb DEPENDS grass_gis)
add_custom_command(
  TARGET grass_symb
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/symbol/symbol
    ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/symbol)
install(DIRECTORY ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/symbol
        DESTINATION ${GRASS_INSTALL_ETCDIR})

add_subdirectory(init)

build_library_in_subdir(cdhc DEPENDS grass_gis grass_raster)

build_library_in_subdir(stats DEPENDS grass_gis grass_raster)

build_library_in_subdir(arraystats DEPENDS grass_gis)

if(WITH_OPENGL)
  build_library_in_subdir(
    ogsf
    DEPENDS
    grass_bitmap
    grass_dbmibase
    grass_dbmiclient
    grass_dbmidriver
    grass_gis
    grass_raster
    grass_raster3d
    grass_vector
    GDAL::GDAL
    OpenGL::GL
    OpenGL::GLU
    TIFF::TIFF)

  build_library_in_subdir(
    nviz
    DEPENDS
    grass_display
    grass_raster
    grass_vector
    grass_bitmap
    grass_ogsf
    grass_gis
    OpenGL::GL
    OPTIONAL_DEPENDS
    X11::X11)
endif()

add_subdirectory(temporal)

build_library_in_subdir(iostream SRC_REGEX "*.cpp" DEPENDS grass_gis)

build_library_in_subdir(manage DEPENDS grass_gis grass_raster grass_vector
                        grass_raster3d GDAL::GDAL)
file(COPY manage/element_list DESTINATION ${OUTDIR}/${GRASS_INSTALL_ETCDIR})
install(FILES ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/element_list
        DESTINATION ${GRASS_INSTALL_ETCDIR})

build_library_in_subdir(calc DEPENDS grass_raster grass_gis)
