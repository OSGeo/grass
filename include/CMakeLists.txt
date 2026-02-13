set(SRCHS
  arraystats.h
  bitmap.h
  btree.h
  calc.h
  cdhc.h
  cluster.h
  colors.h
  datetime.h
  dbmi.h
  display.h
  fontcap.h
  form.h
  gis.h
  glocale.h
  gmath.h
  gprojects.h
  imagery.h
  la.h
  linkm.h
  manage.h
  neta.h
  nviz.h
  ogsf.h
  ortholib.h
  raster.h
  raster3d.h
  rbtree.h
  rowio.h
  segment.h
  spawn.h
  sqlp.h
  stats.h
  symbol.h
  temporal.h
  vector.h
  vedit.h
  defs/arraystats.h
  defs/bitmap.h
  defs/btree.h
  defs/calc.h
  defs/cdhc.h
  defs/cluster.h
  defs/colors.h
  defs/datetime.h
  defs/dbmi.h
  defs/devlib.h
  defs/dig_atts.h
  defs/display.h
  defs/form.h
  defs/gis.h
  defs/glocale.h
  defs/gmath.h
  defs/gprojects.h
  defs/imagery.h
  defs/la.h
  defs/linkm.h
  defs/manage.h
  defs/neta.h
  defs/nviz.h
  defs/ogsf.h
  defs/ortholib.h
  defs/Paintlib.h
  defs/raster.h
  defs/raster3d.h
  defs/rbtree.h
  defs/rowio.h
  defs/segment.h
  defs/spawn.h
  defs/sqlp.h
  defs/stats.h
  defs/symbol.h
  defs/vector.h
  defs/vedit.h
  iostream/ami_config.h
  iostream/ami_sort_impl.h
  iostream/ami_sort.h
  iostream/ami_stream.h
  iostream/ami.h
  iostream/embuffer.h
  iostream/empq_adaptive_impl.h
  iostream/empq_adaptive.h
  iostream/empq_impl.h
  iostream/empq.h
  iostream/imbuffer.h
  iostream/mem_stream.h
  iostream/minmaxheap.h
  iostream/mm_utils.h
  iostream/mm.h
  iostream/pqheap.h
  iostream/queue.h
  iostream/quicksort.h
  iostream/replacementHeap.h
  iostream/replacementHeapBlock.h
  iostream/rtimer.h
  vect/dig_defines.h
  vect/dig_externs.h
  vect/dig_macros.h
  vect/dig_structs.h
  vect/digit.h)

list(TRANSFORM SRCHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/grass/")

set(include_depends)
foreach(srch ${SRCHS})
  get_filename_component(srch_DIR ${srch} DIRECTORY)
  get_filename_component(srch_NAME ${srch} NAME)
  string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" dsth_DIR "${srch_DIR}")
  set(output_dir ${OUTDIR}/${GRASS_INSTALL_INCLUDEDIR}/${dsth_DIR})
  add_custom_command(
    OUTPUT ${output_dir}/${srch_NAME}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${output_dir}
    COMMAND ${CMAKE_COMMAND} -E copy ${srch} ${output_dir}
    COMMENT "Copy ${srch} to ${output_dir}/${srch_NAME}")
  list(APPEND include_depends ${output_dir}/${srch_NAME})
endforeach()

add_custom_target(INCLUDE_HEADERS DEPENDS ${include_depends} LIB_PYTHON)
target_sources(INCLUDE_HEADERS PRIVATE ${SRCHS} config.h.cmake.in grass/version.h.in)
set_target_properties(INCLUDE_HEADERS PROPERTIES FOLDER "GRASS Libraries")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/grass/version.h.in
               "${OUTDIR}/${GRASS_INSTALL_INCLUDEDIR}/grass/version.h")

message(STATUS "Creating ${OUTDIR}/${GRASS_INSTALL_INCLUDEDIR}/grass/config.h")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake.in
               "${OUTDIR}/${GRASS_INSTALL_INCLUDEDIR}/grass/config.h")

# TODO: this presently skips empty lines, but should be included
file(WRITE ${OUTDIR}/${GRASS_INSTALL_INCLUDEDIR}/grass/copying.h "")
file(STRINGS ${CMAKE_SOURCE_DIR}/COPYING copying_lines)
foreach(copying_line ${copying_lines})
  file(APPEND ${OUTDIR}/${GRASS_INSTALL_INCLUDEDIR}/grass/copying.h
       "\"${copying_line} \\n\"\n")
endforeach()

# TODO: this presently skips empty lines, but should be included
file(WRITE ${OUTDIR}/${GRASS_INSTALL_INCLUDEDIR}/grass/citing.h "")
file(STRINGS ${CMAKE_SOURCE_DIR}/CITING citing_lines)
foreach(citing_line ${citing_lines})
  file(APPEND ${OUTDIR}/${GRASS_INSTALL_INCLUDEDIR}/grass/citing.h
       "\"${citing_line}\\n\"\n")
endforeach()

# TODO file(READ  ${CMAKE_SOURCE_DIR}/config.status config_status_header)
file(WRITE ${OUTDIR}/${GRASS_INSTALL_INCLUDEDIR}/grass/confparms.h
     "\"/*  */\\n\"")

install(DIRECTORY ${OUTDIR}/${GRASS_INSTALL_INCLUDEDIR}/
        DESTINATION ${GRASS_INSTALL_INCLUDEDIR})
