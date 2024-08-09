file(GLOB_RECURSE SRCHS "*.h")
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

add_custom_target(copy_header DEPENDS ${include_depends} LIB_PYTHON)

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
