add_subdirectory(Make)

file(GLOB_RECURSE SRCHS "*.h" "*.make")
set(include_depends)
foreach(srch ${SRCHS} "Make/Platform.make")
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

# add_subdirectory(Make)

# file(GLOB_RECURSE SRCMAKES "*.make")
# set(include_make_depends)
# foreach(srcmake ${SRCMAKES})
#   get_filename_component(srcmake_DIR ${srcmake} DIRECTORY)
#   get_filename_component(srcmake_NAME ${srcmake} NAME)
#   string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" dstmake_DIR "${srcmake_DIR}")
#   set(output_dir1 ${OUTDIR}/${GRASS_INSTALL_INCLUDEDIR}/${dstmake_DIR})
#   add_custom_command(
#     OUTPUT ${output_dir1}/${srcmake_NAME}
#     COMMAND ${CMAKE_COMMAND} -E make_directory ${output_dir1}
#     COMMAND ${CMAKE_COMMAND} -E copy ${srcmake} ${output_dir1}
#     COMMENT "Copy ${srcmake} to ${output_dir1}/${srcmake_NAME}")
#   list(APPEND include_make_depends ${output_dir1}/${srcmake_NAME})
# endforeach()

# add_custom_target(copy_make DEPENDS ${include_make_depends} LIB_PYTHON)

install(DIRECTORY ${OUTDIR}/${GRASS_INSTALL_INCLUDEDIR}/
        DESTINATION ${GRASS_INSTALL_INCLUDEDIR})
