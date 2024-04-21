file(GLOB_RECURSE SRCHS "*.h")
set(include_depends)
foreach(srch ${SRCHS})
  get_filename_component(srch_DIR ${srch} DIRECTORY)
  get_filename_component(srch_NAME ${srch} NAME)
  string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" dsth_DIR "${srch_DIR}")
  set(output_dir ${GISBASE}/include/${dsth_DIR})
  add_custom_command(
    OUTPUT ${output_dir}/${srch_NAME}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${output_dir}
    COMMAND ${CMAKE_COMMAND} -E copy ${srch} ${output_dir}
    COMMENT "Copy ${srch} to ${output_dir}/${srch_NAME}")
  list(APPEND include_depends ${output_dir}/${srch_NAME})
endforeach()

add_custom_target(copy_header DEPENDS ${include_depends} LIB_PYTHON)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/grass/version.h.in
               "${GISBASE}/include/grass/version.h")

message(STATUS "Creating ${GISBASE}/include/grass/config.h")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake.in
               "${GISBASE}/include/grass/config.h")

file(WRITE ${GISBASE}/include/grass/copying.h "")
file(STRINGS ${CMAKE_SOURCE_DIR}/COPYING copying_lines)
foreach(copying_line ${copying_lines})
  file(APPEND ${GISBASE}/include/grass/copying.h "\"${copying_line} \\n\"\n")
endforeach()

file(WRITE ${GISBASE}/include/grass/citing.h "")
file(STRINGS ${CMAKE_SOURCE_DIR}/CITING citing_lines)
foreach(citing_line ${citing_lines})
  file(APPEND ${GISBASE}/include/grass/citing.h "\"${citing_line}\\n\"\n")
endforeach()

# TODO file(READ  ${CMAKE_SOURCE_DIR}/config.status config_status_header)
file(WRITE ${GISBASE}/include/grass/confparms.h "\"/*  */\\n\"")
