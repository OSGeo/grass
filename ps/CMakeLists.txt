build_program_in_subdir(
  ps.map
  DEPENDS
  grass_gis
  grass_raster
  grass_gmath
  grass_gproj
  grass_imagery
  grass_vector
  grass_dbmibase
  grass_dbmiclient
  grass_dbmidriver
  grass_symb
  ${LIBM})

add_custom_command(
  TARGET ps.map
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory
          ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/paint/decorations
  COMMAND ${CMAKE_COMMAND} -E make_directory
          ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/paint/patterns
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/ps.map/prolog.ps
          ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/paint
  COMMAND
    ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/ps.map/decorations
    ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/paint/decorations
  COMMAND
    ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/ps.map/patterns
    ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/paint/patterns)

install(DIRECTORY ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/paint
        DESTINATION ${GRASS_INSTALL_ETCDIR})
