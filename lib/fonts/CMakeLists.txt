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
list(TRANSFORM font_files PREPEND fonts/ OUTPUT_VARIABLE font_files_fonts)
list(TRANSFORM font_files PREPEND ${OUTDIR}/${GRASS_INSTALL_FONTSDIR}/
                                  OUTPUT_VARIABLE font_files_outdir)
foreach(font_file ${font_files})
  add_custom_command(
    OUTPUT ${OUTDIR}/${GRASS_INSTALL_FONTSDIR}/${font_file}
    COMMAND
      ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/fonts/${font_file}"
      ${OUTDIR}/${GRASS_INSTALL_FONTSDIR}
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/fonts/${font_file}")
endforeach()

add_custom_target(fonts DEPENDS ${font_files_outdir})
install(FILES ${font_files_fonts} DESTINATION ${GRASS_INSTALL_FONTSDIR})
# foreach(font_file ${font_files})
#   cmake_path(GET font_file FILENAME font_file_filename)
#   add_custom_command(
#     OUTPUT
#       "$<PATH:ABSOLUTE_PATH,${font_files},${OUTDIR}/${GRASS_INSTALL_FONTSDIR}>"
#     COMMAND_EXPAND_LISTS
#     # $<JOIN:$<PATH:ABSOLUTE_PATH,$<PATH:CMAKE_PATH,NORMALIZE,${font_files}>,${CMAKE_CURRENT_SOURCE_DIR}/fonts>, >
#     # "$<JOIN:$<PATH:ABSOLUTE_PATH,$<PATH:CMAKE_PATH,NORMALIZE,${font_files}>,${CMAKE_CURRENT_SOURCE_DIR}/fonts>, >"
#     # "$<JOIN:$<PATH:ABSOLUTE_PATH,${font_files},${CMAKE_CURRENT_SOURCE_DIR}/fonts>,; >"
#     COMMAND
#       ${CMAKE_COMMAND} -E copy_if_different
#       "$<PATH:ABSOLUTE_PATH,${font_files},${CMAKE_CURRENT_SOURCE_DIR}/fonts>"
#       ${OUTDIR}/${GRASS_INSTALL_FONTSDIR}
#     DEPENDS
#       "$<PATH:ABSOLUTE_PATH,${font_files},${CMAKE_CURRENT_SOURCE_DIR}/fonts>")
# endforeach()
# set(font_files2 ${font_files})
# list(TRANSFORM font_files PREPEND fonts/)

# list(TRANSFORM font_files )
# add_custom_target()
# $<PATH:ABSOLUTE_PATH[,NORMALIZE],path...,base_directory>
# add_custom_command(
#   OUTPUT
#     "$<PATH:ABSOLUTE_PATH,${font_files},${OUTDIR}/${GRASS_INSTALL_FONTSDIR}>"
#   COMMAND_EXPAND_LISTS
#   # $<JOIN:$<PATH:ABSOLUTE_PATH,$<PATH:CMAKE_PATH,NORMALIZE,${font_files}>,${CMAKE_CURRENT_SOURCE_DIR}/fonts>, >
#   # "$<JOIN:$<PATH:ABSOLUTE_PATH,$<PATH:CMAKE_PATH,NORMALIZE,${font_files}>,${CMAKE_CURRENT_SOURCE_DIR}/fonts>, >"
#   # "$<JOIN:$<PATH:ABSOLUTE_PATH,${font_files},${CMAKE_CURRENT_SOURCE_DIR}/fonts>,; >"
#   COMMAND
#     ${CMAKE_COMMAND} -E copy_if_different
#     "$<PATH:ABSOLUTE_PATH,${font_files},${CMAKE_CURRENT_SOURCE_DIR}/fonts>"
#     ${OUTDIR}/${GRASS_INSTALL_FONTSDIR}
#   DEPENDS
#     "$<PATH:ABSOLUTE_PATH,${font_files},${CMAKE_CURRENT_SOURCE_DIR}/fonts>")
# add_custom_command(
#     OUTPUT "$<PATH:ABSOLUTE_PATH,${font_files},${OUTDIR}/${GRASS_INSTALL_FONTSDIR}>"
#     COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<PATH:ABSOLUTE_PATH,${font_files},${CMAKE_CURRENT_SOURCE_DIR}/fonts>"
#             ${OUTDIR}/${GRASS_INSTALL_FONTSDIR}
#     DEPENDS "$<PATH:ABSOLUTE_PATH,${font_files},${CMAKE_CURRENT_SOURCE_DIR}/fonts>"
#     BYPRODUCTS "$<PATH:ABSOLUTE_PATH,${font_files},${OUTDIR}/${GRASS_INSTALL_FONTSDIR}>")
# add_custom_target(
#   fonts
#   DEPENDS
#     "$<PATH:ABSOLUTE_PATH,${font_files},${OUTDIR}/${GRASS_INSTALL_FONTSDIR}>")
# install(
#   FILES
#     "$<PATH:ABSOLUTE_PATH,${font_files},${OUTDIR}/${GRASS_INSTALL_FONTSDIR}>"
#   DESTINATION ${GRASS_INSTALL_FONTSDIR})
# add_custom_command(
#     OUTPUT "$<PATH:ABSOLUTE_PATH,${font_files2},${OUTDIR}/${GRASS_INSTALL_FONTSDIR}>"
#     COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<PATH:ABSOLUTE_PATH,${font_files},${CMAKE_CURRENT_SOURCE_DIR}>"
#             ${OUTDIR}/${GRASS_INSTALL_FONTSDIR}
#     DEPENDS "$<PATH:ABSOLUTE_PATH,${font_files},${CMAKE_CURRENT_SOURCE_DIR}>"
#     BYPRODUCTS "$<PATH:ABSOLUTE_PATH,${font_files2},${OUTDIR}/${GRASS_INSTALL_FONTSDIR}>")
# add_custom_target(fonts DEPENDS "$<PATH:ABSOLUTE_PATH,${font_files2},${OUTDIR}/${GRASS_INSTALL_FONTSDIR}>")

# # add_custom_command(
# #   TARGET fonts
# #   PRE_BUILD
# #   COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTDIR}/${GRASS_INSTALL_FONTSDIR}
# #   BYPRODUCTS ${OUTDIR}/${GRASS_INSTALL_FONTSDIR})

# foreach(font_file ${font_files})
#   # TODO: Use $<PATH:GET_FILENAME,path...> generator expression with cmake 3.24
#   # https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html#path-decomposition
#   cmake_path(GET font_file FILENAME font_file_filename)
#   add_custom_command(
#     OUTPUT ${OUTDIR}/${GRASS_INSTALL_FONTSDIR}/${font_file_filename}
#     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${font_file}
#             ${OUTDIR}/${GRASS_INSTALL_FONTSDIR}
#     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${font_file} ${font_file} ${OUTDIR}/${GRASS_INSTALL_FONTSDIR}
#     BYPRODUCTS ${OUTDIR}/${GRASS_INSTALL_FONTSDIR}/${font_file_filename})
#   # add_custom_command(
#   #   TARGET fonts
#   #   PRE_BUILD
#   #   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${font_file}
#   #           ${OUTDIR}/${GRASS_INSTALL_FONTSDIR}
#   #   # DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${font_file}
#   #   BYPRODUCTS ${OUTDIR}/${GRASS_INSTALL_FONTSDIR}/${font_file_filename})
#   # add_dependencies(fonts ${OUTDIR}/${GRASS_INSTALL_FONTSDIR}/${font_file_filename})
#   # add_dependencies(fonts ${CMAKE_CURRENT_SOURCE_DIR}/${font_file})
# endforeach()
# unset(font_file_filename)

# install(FILES ${font_files} DESTINATION ${GRASS_INSTALL_FONTSDIR})
# install(FILES "$<PATH:ABSOLUTE_PATH,${font_files2},${OUTDIR}/${GRASS_INSTALL_FONTSDIR}>" DESTINATION ${GRASS_INSTALL_FONTSDIR})
