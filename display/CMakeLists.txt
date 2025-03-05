set(display_modules_list
    d.barscale
    d.colorlist
    d.colortable
    d.erase
    d.font
    d.fontlist
    d.geodesic
    d.graph
    d.grid
    d.his
    d.histogram
    d.info
    d.labels
    d.legend
    d.legend.vect
    d.linegraph
    d.mon
    d.northarrow
    d.path
    d.profile
    d.rast
    d.rast.arrow
    d.rast.num
    d.redraw
    d.rgb
    d.rhumbline
    d.text
    d.title
    d.vect
    d.vect.chart
    d.vect.thematic
    d.where)

if(MSVC)
  list(REMOVE_ITEM display_modules_list d.font d.mon)
endif()

add_custom_target(
  ALL_DISPLAY_MODULES ALL
  DEPENDS ${display_modules_list}
  COMMENT "All display modules are built.")

if(WITH_DOCS)
  generate_html(TARGET ALL_DISPLAY_MODULES NAME displaydrivers)
endif()

build_program_in_subdir(d.barscale DEPENDS grass_symb grass_display grass_gis)
file(GLOB d_barscale_png
     ${CMAKE_CURRENT_SOURCE_DIR}/d.barscale/thumbnails/*.png)
add_custom_command(
  TARGET d.barscale
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory
          ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/barscales
  COMMAND ${CMAKE_COMMAND} -E copy ${d_barscale_png}
          ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/barscales
  BYPRODUCTS ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/barscales)
install(DIRECTORY ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/barscales
        DESTINATION ${GRASS_INSTALL_DOCDIR})

build_program_in_subdir(d.colorlist DEPENDS grass_gis grass_display
                        grass_raster)
build_program_in_subdir(d.colortable DEPENDS grass_gis grass_display
                        grass_raster ${LIBM})
build_program_in_subdir(d.erase DEPENDS grass_gis grass_display)
if(NOT MSVC)
  build_program_in_subdir(d.font DEPENDS grass_gis grass_display grass_raster)
  # libgen.h does not exists on windows msvc
  build_program_in_subdir(d.mon DEPENDS grass_gis grass_display)
  copy_python_files_in_subdir(d.mon ${GRASS_INSTALL_ETCBINDIR} TARGET d.mon
                              POST_BUILD)
endif()
build_program_in_subdir(d.fontlist DEPENDS grass_gis grass_display grass_raster)
build_program_in_subdir(d.geodesic DEPENDS grass_gis grass_display)
build_program_in_subdir(d.graph DEPENDS grass_gis grass_symb grass_display)
if(WITH_DOCS)
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/d.graph/grass_logo.txt
       DESTINATION ${OUTDIR}/${GRASS_INSTALL_DOCDIR})
  install(FILES ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/grass_logo.txt
          DESTINATION ${GRASS_INSTALL_DOCDIR})
endif()
build_program_in_subdir(
  d.grid
  DEPENDS
  grass_gis
  grass_symb
  grass_gproj
  grass_display
  ${LIBM})
build_program_in_subdir(d.his DEPENDS grass_gis grass_display grass_raster)
build_program_in_subdir(d.histogram DEPENDS grass_gis grass_display
                        grass_raster ${LIBM})
build_program_in_subdir(d.info DEPENDS grass_gis grass_display)
build_program_in_subdir(d.labels DEPENDS grass_gis grass_display ${LIBM})
build_program_in_subdir(
  d.legend
  DEPENDS
  grass_gis
  grass_display
  grass_raster
  grass_raster3d
  ${LIBM})
build_program_in_subdir(d.legend.vect DEPENDS grass_gis grass_display
                        grass_symb ${LIBM})
build_program_in_subdir(d.linegraph DEPENDS grass_gis grass_display grass_symb
                        grass_raster)
build_program_in_subdir(d.northarrow DEPENDS grass_gis grass_display grass_symb
                        ${LIBM})
if(WITH_DOCS)
  file(MAKE_DIRECTORY ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/northarrows)
  file(GLOB d_northarrow_png
       ${CMAKE_CURRENT_SOURCE_DIR}/d.northarrow/thumbnails/*.png)
  file(COPY ${d_northarrow_png}
       DESTINATION ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/northarrows)
  install(DIRECTORY ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/northarrows
          DESTINATION ${GRASS_INSTALL_DOCDIR})
endif()
build_program_in_subdir(d.path DEPENDS grass_gis grass_display grass_vector)
build_program_in_subdir(d.profile DEPENDS grass_gis grass_display grass_raster
                        ${LIBM})
build_program_in_subdir(d.rast DEPENDS grass_gis grass_display grass_raster)
build_program_in_subdir(d.rast.arrow DEPENDS grass_gis grass_raster
                        grass_display ${LIBM})
build_program_in_subdir(d.rast.num DEPENDS grass_gis grass_display grass_raster)
build_program_in_subdir(d.redraw DEPENDS grass_gis grass_display)
build_program_in_subdir(d.rgb DEPENDS grass_gis grass_display grass_raster)
build_program_in_subdir(d.rhumbline DEPENDS grass_gis grass_display)
build_program_in_subdir(d.text DEPENDS grass_gis grass_display ${LIBM})
build_program_in_subdir(d.title DEPENDS grass_gis grass_display grass_raster)
build_program_in_subdir(
  d.vect
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_display
  grass_gis
  grass_raster
  grass_symb
  grass_vector
  ${LIBM})
build_program_in_subdir(
  d.vect.chart
  DEPENDS
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_display
  grass_gis
  grass_symb
  grass_vector
  ${LIBM})
build_program_in_subdir(
  d.vect.thematic
  DEPENDS
  grass_arraystats
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_display
  grass_gis
  grass_symb
  grass_vector)
build_program_in_subdir(d.where DEPENDS grass_gis grass_display grass_gproj)
