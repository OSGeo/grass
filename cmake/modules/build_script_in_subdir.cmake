#[[
AUTHOR(S):  Rashad Kanavath <rashad km gmail>
PURPOSE:    A CMake function that builds grass python script modules
COPYRIGHT:  (C) 2020 by the GRASS Development Team

SPDX-License-Identifier: GPL-2.0-or-later
#]]

function(build_script_in_subdir dir_name)

  cmake_parse_arguments(
    G
    "PLAIN_PY;NO_DOCS"
    "DEST_DIR"
    ""
    ${ARGN})

  if(NOT G_DEST_DIR)
    set(G_DEST_DIR ${GRASS_INSTALL_SCRIPTDIR})
  endif()

  set(G_NAME ${dir_name})

  if(G_PLAIN_PY)
    set(G_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  else()
    set(G_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${G_NAME})
  endif()

  file(GLOB PYTHON_FILES "${G_SRC_DIR}/*.py")
  if(NOT PYTHON_FILES)
    message(FATAL_ERROR "[${G_NAME}]: No PYTHON_FILES found.")
  endif()

  set(SRC_SCRIPT_FILE ${G_SRC_DIR}/${G_NAME}.py)

  if(NOT EXISTS ${SRC_SCRIPT_FILE})
    message(FATAL_ERROR "${SRC_SCRIPT_FILE} does not exists")
    return()
  endif()

  set(script_ext "")
  if(WIN32)
    set(script_ext ".py")
  endif()

  set(HTML_FILE ${G_SRC_DIR}/${G_NAME}.html)

  configure_file(
    ${G_SRC_DIR}/${G_NAME}.py
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${G_NAME}${script_ext} COPYONLY)
  file(
    COPY ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${G_NAME}${script_ext}
    DESTINATION ${OUTDIR}/${G_DEST_DIR}
    FILE_PERMISSIONS
      OWNER_READ
      OWNER_WRITE
      OWNER_EXECUTE
      GROUP_READ
      GROUP_EXECUTE
      WORLD_READ
      WORLD_EXECUTE)

  set(TRANSLATE_C_FILE
      ${CMAKE_SOURCE_DIR}/locale/scriptstrings/${G_NAME}_to_translate.c)

  add_custom_command(
    OUTPUT ${TRANSLATE_C_FILE}
    COMMAND
      ${CMAKE_COMMAND} -DG_NAME=${G_NAME} -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
      -DOUTPUT_FILE=${TRANSLATE_C_FILE} -DGISBASE_DIR="${RUNTIME_GISBASE}"
      -DBINARY_DIR="${OUTDIR}/${GRASS_INSTALL_BINDIR}"
      -DLIBDIR="${OUTDIR}/${GRASS_INSTALL_LIBDIR}"
      -DSCRIPTDIR="${OUTDIR}/${G_DEST_DIR}"
      -DETCDIR="${OUTDIR}/${GRASS_INSTALL_ETCDIR}"
      -DPYDIR="${OUTDIR}/${GRASS_INSTALL_PYDIR}" -DGISRC="${GISRC}"
      -DGUIDIR="${OUTDIR}/${GRASS_INSTALL_GUIDIR}" -P
      ${CMAKE_SOURCE_DIR}/cmake/locale_strings.cmake
    DEPENDS g.parser)

  set(HTML_FILE_NAME ${G_NAME})
  set(OUT_HTML_FILE "")

  if(WITH_DOCS AND NOT G_NO_DOCS)

    file(GLOB IMG_FILES ${G_SRC_DIR}/*.png ${G_SRC_DIR}/*.jpg)
    if(IMG_FILES)
      set(copy_images_command ${CMAKE_COMMAND} -E copy ${IMG_FILES}
                              ${OUTDIR}/${GRASS_INSTALL_DOCDIR})
      install(FILES ${IMG_FILES} DESTINATION ${GRASS_INSTALL_DOCDIR})
    endif()

    set(out_html_file ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/${G_NAME}.html)

    generate_docs(${G_NAME}
      OUTPUT ${out_html_file}
      SOURCEDIR ${G_SRC_DIR}
      DEST_DIR ${G_DEST_DIR}
      DEPENDS ${TRANSLATE_C_FILE} LIB_PYTHON)

  endif() # WITH_DOCS

  add_custom_target(${G_NAME} DEPENDS ${TRANSLATE_C_FILE} ${out_html_file})

  set(modules_list
      "${G_NAME};${modules_list}"
      CACHE INTERNAL "list of modules")

 if("${G_NAME}" MATCHES "^v[\.]")
   set_target_properties(${G_NAME} PROPERTIES FOLDER "Tools/Vector")
 elseif("${G_NAME}" MATCHES "^r[\.]")
   set_target_properties(${G_NAME} PROPERTIES FOLDER "Tools/Raster")
 elseif("${G_NAME}" MATCHES "^d[\.]")
   set_target_properties(${G_NAME} PROPERTIES FOLDER "Tools/Display")
 elseif("${G_NAME}" MATCHES "^db[\.]")
   set_target_properties(${G_NAME} PROPERTIES FOLDER "Tools/Database")
 elseif("${G_NAME}" MATCHES "^g[\.]")
   set_target_properties(${G_NAME} PROPERTIES FOLDER "Tools/General")
 elseif("${G_NAME}" MATCHES "^i[\.]")
   set_target_properties(${G_NAME} PROPERTIES FOLDER "Tools/Imagery")
 elseif("${G_NAME}" MATCHES "^m[\.]")
   set_target_properties(${G_NAME} PROPERTIES FOLDER "Tools/Miscellaneous")
 elseif("${G_NAME}" MATCHES "^ps[\.]")
   set_target_properties(${G_NAME} PROPERTIES FOLDER "Tools/PostScript")
 elseif("${G_NAME}" MATCHES "^r3[\.]")
   set_target_properties(${G_NAME} PROPERTIES FOLDER "Tools/Raster 3D")
 elseif("${G_NAME}" MATCHES "^t[\.]")
    set_target_properties(${G_NAME} PROPERTIES FOLDER "Tools/Temporal")
 else()
   set_target_properties(${G_NAME} PROPERTIES FOLDER "Binaries")
 endif()

  if(WIN32)
    install(PROGRAMS ${OUTDIR}/${G_DEST_DIR}/${G_NAME}.bat
            DESTINATION ${G_DEST_DIR})
  endif()

  install(PROGRAMS ${OUTDIR}/${G_DEST_DIR}/${G_NAME}${script_ext}
          DESTINATION ${G_DEST_DIR})

  file(GLOB _files
       LIST_DIRECTORIES FALSE
       ${G_SRC_DIR}/*.py
       ${G_SRC_DIR}/*.html ${G_SRC_DIR}/*.md
       ${G_SRC_DIR}/*.png ${G_SRC_DIR}/*.jpg)
  target_sources(${G_NAME} PRIVATE ${_files})


endfunction()
