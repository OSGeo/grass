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

  set(SCRIPT_EXT "")
  if(WIN32)
    set(SCRIPT_EXT ".py")
  endif()

  set(HTML_FILE ${G_SRC_DIR}/${G_NAME}.html)

  configure_file(
    ${G_SRC_DIR}/${G_NAME}.py
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${G_NAME}${SCRIPT_EXT} COPYONLY)
  file(
    COPY ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${G_NAME}${SCRIPT_EXT}
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

    set(HTML_FILE ${G_SRC_DIR}/${G_NAME}.html)
    if(EXISTS ${HTML_FILE})
      install(FILES ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/${G_NAME}.html
              DESTINATION ${GRASS_INSTALL_DOCDIR})
    else()
      set(HTML_FILE)
      file(GLOB html_files ${G_SRC_DIR}/*.html)
      if(html_files)
        message(
          FATAL_ERROR
            "${html_file} does not exists. ${G_SRC_DIR} \n ${OUTDIR}/${G_DEST_DIR}| ${G_NAME}"
        )
      endif()
    endif()

    set(TMP_HTML_FILE ${CMAKE_CURRENT_BINARY_DIR}/${G_NAME}.tmp.html)
    set(OUT_HTML_FILE ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/${G_NAME}.html)

    add_custom_command(
      OUTPUT ${OUT_HTML_FILE}
      COMMAND ${CMAKE_COMMAND} -E copy ${G_SRC_DIR}/${G_NAME}.html
              ${CMAKE_CURRENT_BINARY_DIR}/${G_NAME}.html
      COMMAND
        ${grass_env_command} ${PYTHON_EXECUTABLE}
        ${OUTDIR}/${G_DEST_DIR}/${G_NAME}${SCRIPT_EXT}
        --html-description < ${NULL_DEVICE} | ${SEARCH_COMMAND}
        ${HTML_SEARCH_STR} > ${TMP_HTML_FILE}
      COMMAND ${grass_env_command} ${PYTHON_EXECUTABLE} ${MKHTML_PY} ${G_NAME} >
              ${OUT_HTML_FILE}
      COMMAND ${copy_images_command}
      COMMAND ${CMAKE_COMMAND} -E remove ${TMP_HTML_FILE}
              ${CMAKE_CURRENT_BINARY_DIR}/${G_NAME}.html
      COMMENT "Creating ${OUT_HTML_FILE}"
      DEPENDS ${TRANSLATE_C_FILE} LIB_PYTHON)

    install(FILES ${OUT_HTML_FILE} DESTINATION ${GRASS_INSTALL_DOCDIR})

  endif() # WITH_DOCS

  add_custom_target(${G_NAME} DEPENDS ${TRANSLATE_C_FILE} ${OUT_HTML_FILE})

  set(modules_list
      "${G_NAME};${modules_list}"
      CACHE INTERNAL "list of modules")

  set_target_properties(${G_NAME} PROPERTIES FOLDER scripts)

  if(WIN32)
    install(PROGRAMS ${OUTDIR}/${G_DEST_DIR}/${G_NAME}.bat
            DESTINATION ${G_DEST_DIR})
  endif()

  install(PROGRAMS ${OUTDIR}/${G_DEST_DIR}/${G_NAME}${SCRIPT_EXT}
          DESTINATION ${G_DEST_DIR})

endfunction()
