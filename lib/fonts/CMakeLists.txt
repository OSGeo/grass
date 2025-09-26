set(font_files
    cyrilc.hmp
    fonts.table
    gothgbt.hmp
    gothgrt.hmp
    gothitt.hmp
    greekc.hmp
    greekcs.hmp
    greekp.hmp
    greeks.hmp
    hersh.oc1
    hersh.oc2
    hersh.oc3
    hersh.oc4
    italicc.hmp
    italiccs.hmp
    italict.hmp
    romanc.hmp
    romancs.hmp
    romand.hmp
    romans.hmp
    romant.hmp
    scriptc.hmp
    scripts.hmp)
list(TRANSFORM font_files PREPEND fonts/)

add_custom_target(fonts)

add_custom_command(
  TARGET fonts
  PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTDIR}/${GRASS_INSTALL_FONTSDIR}
  BYPRODUCTS ${OUTDIR}/${GRASS_INSTALL_FONTSDIR})

foreach(font_file ${font_files})
  add_custom_command(
    TARGET fonts
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${font_file}
            ${OUTDIR}/${GRASS_INSTALL_FONTSDIR})
endforeach()

install(FILES ${font_files} DESTINATION ${GRASS_INSTALL_FONTSDIR})
