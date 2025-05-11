#[[
AUTHOR(S):  Rashad Kanavath <rashad km gmail>
PURPOSE:    build_gui_in_subdir is the cmake function that builds g.gui.* modules
COPYRIGHT:  (C) 2020 by the GRASS Development Team

SPDX-License-Identifier: GPL-2.0-or-later
#]]

function(build_gui_in_subdir dir_name)
  set(G_NAME ${dir_name})
  set(G_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${G_NAME})
  set(G_TARGET_NAME g.gui.${G_NAME})

  set(HTML_FILE_NAME ${G_TARGET_NAME})
  set(MD_FILE_NAME ${G_TARGET_NAME})

  file(GLOB PYTHON_FILES "${G_SRC_DIR}/*.py")
  if(NOT PYTHON_FILES)
    message(FATAL_ERROR "[${G_TARGET_NAME}]: No PYTHON_FILES found.")
  endif()

  set(SRC_SCRIPT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${G_NAME}/${G_TARGET_NAME}.py)

  if(NOT EXISTS ${SRC_SCRIPT_FILE})
    message(FATAL_ERROR "${SRC_SCRIPT_FILE} does not exists")
  endif()

  set(SCRIPT_EXT "")
  if(WIN32)
    set(SCRIPT_EXT ".py")
    set(PGM_NAME ${G_TARGET_NAME})
    configure_file(${CMAKE_SOURCE_DIR}/cmake/windows_launch.bat.in
                   ${OUTDIR}/${GRASS_INSTALL_SCRIPTDIR}/${G_TARGET_NAME}.bat @ONLY)
  endif()
  set(GUI_STAMP_FILE ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${G_NAME}.stamp)

  add_custom_command(
    OUTPUT ${GUI_STAMP_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "${OUTDIR}/${GRASS_INSTALL_GUIDIR}/wxpython/${G_NAME}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PYTHON_FILES}
            "${OUTDIR}/${GRASS_INSTALL_GUIDIR}/wxpython/${G_NAME}"
    COMMAND ${CMAKE_COMMAND} -E touch ${GUI_STAMP_FILE})

  set(OUT_SCRIPT_FILE
      "${OUTDIR}/${GRASS_INSTALL_SCRIPTDIR}/${G_TARGET_NAME}${SCRIPT_EXT}")

  if(UNIX)
  add_custom_command(
    OUTPUT ${OUT_SCRIPT_FILE}
    COMMAND ${CMAKE_COMMAND} -E copy ${SRC_SCRIPT_FILE} ${OUT_SCRIPT_FILE}
    COMMAND /bin/chmod 755 ${OUT_SCRIPT_FILE}
    DEPENDS g.parser ${SRC_SCRIPT_FILE})
  else()
  add_custom_command(
    OUTPUT ${OUT_SCRIPT_FILE}
    COMMAND ${CMAKE_COMMAND} -E copy ${SRC_SCRIPT_FILE} ${OUT_SCRIPT_FILE}
    DEPENDS g.parser ${SRC_SCRIPT_FILE})
  endif()

  if(WITH_DOCS)

    file(GLOB IMG_FILES ${G_SRC_DIR}/*.png ${G_SRC_DIR}/*.jpg)
    if(IMG_FILES)
      set(copy_images_command ${CMAKE_COMMAND} -E copy ${IMG_FILES}
                              "${OUTDIR}/${GRASS_INSTALL_DOCDIR}")
      set(copy_images_md_command ${CMAKE_COMMAND} -E copy ${IMG_FILES}
                                 "${OUTDIR}/${GRASS_INSTALL_MKDOCSDIR}")
      install(FILES ${IMG_FILES} DESTINATION ${GRASS_INSTALL_DOCDIR})
      install(FILES ${IMG_FILES} DESTINATION ${GRASS_INSTALL_MKDOCSDIR})
    endif()

    set(HTML_FILE ${G_SRC_DIR}/${G_TARGET_NAME}.html)
    set(MD_FILE ${G_SRC_DIR}/${G_TARGET_NAME}.md)
    if(EXISTS ${HTML_FILE})
      install(FILES ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/${G_TARGET_NAME}.html
              DESTINATION ${GRASS_INSTALL_DOCDIR})
    else()
      set(HTML_FILE)
      file(GLOB html_files ${G_SRC_DIR}/*.html)
      if(html_files)
        message(
          FATAL_ERROR
            "${html_file} does not exists. ${G_SRC_DIR} \n ${RUNTIME_GISBASE}/scripts| ${G_TARGET_NAME}"
        )
      endif()
    endif()
    if(EXISTS ${MD_FILE})
      install(FILES ${OUTDIR}/${GRASS_INSTALL_MKDOCSDIR}/${G_TARGET_NAME}.md
              DESTINATION ${GRASS_INSTALL_MKDOCSDIR})
    else()
      set(MD_FILE)
      file(GLOB md_files ${G_SRC_DIR}/*.md)
      if(md_files)
        message(
          FATAL_ERROR
            "${md_file} does not exists. ${G_SRC_DIR} \n ${RUNTIME_GISBASE}/scripts| ${G_TARGET_NAME}"
        )
      endif()
    endif()

    set(TMP_HTML_FILE
        ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${G_TARGET_NAME}.tmp.html)
    set(OUT_HTML_FILE ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/${G_TARGET_NAME}.html)
    set(GUI_HTML_FILE ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/wxGUI.${G_NAME}.html)
    set(TMP_MD_FILE
        ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${G_TARGET_NAME}.tmp.md)
    set(OUT_MD_FILE ${OUTDIR}/${GRASS_INSTALL_MKDOCSDIR}/${G_TARGET_NAME}.md)
    set(GUI_MD_FILE ${OUTDIR}/${GRASS_INSTALL_MKDOCSDIR}/wxGUI.${G_NAME}.md)

    add_custom_command(
      OUTPUT ${OUT_HTML_FILE}
      COMMAND ${CMAKE_COMMAND} -E copy ${G_SRC_DIR}/${G_TARGET_NAME}.html
              ${CMAKE_CURRENT_BINARY_DIR}/${G_TARGET_NAME}.html
      COMMAND
        ${grass_env_command} ${PYTHON_EXECUTABLE}
        ${OUTDIR}/${GRASS_INSTALL_SCRIPTDIR}/${G_TARGET_NAME}${SCRIPT_EXT}
        --html-description < ${NULL_DEVICE} | ${SEARCH_COMMAND}
        ${HTML_SEARCH_STR} > ${TMP_HTML_FILE}
      COMMAND ${grass_env_command} ${PYTHON_EXECUTABLE} ${MKHTML_PY}
              ${G_TARGET_NAME} ${GRASS_VERSION_DATE} > ${OUT_HTML_FILE}
      COMMAND ${copy_images_command}
      COMMAND ${CMAKE_COMMAND} -E remove ${TMP_HTML_FILE}
              ${CMAKE_CURRENT_BINARY_DIR}/${G_TARGET_NAME}.html
      COMMAND ${grass_env_command} ${PYTHON_EXECUTABLE} ${MKHTML_PY}
              ${G_TARGET_NAME} ${GRASS_VERSION_DATE} > ${GUI_HTML_FILE}
      COMMENT "Creating ${OUT_HTML_FILE} and ${GUI_HTML_FILE}"
      DEPENDS ${OUT_SCRIPT_FILE} GUI_WXPYTHON LIB_PYTHON)

    add_custom_command(
      OUTPUT ${OUT_MD_FILE}
      COMMAND ${CMAKE_COMMAND} -E copy ${G_SRC_DIR}/${G_TARGET_NAME}.md
              ${CMAKE_CURRENT_BINARY_DIR}/${G_TARGET_NAME}.md
      COMMAND
        ${grass_env_command} ${PYTHON_EXECUTABLE}
        ${OUTDIR}/${GRASS_INSTALL_SCRIPTDIR}/${G_TARGET_NAME}${SCRIPT_EXT}
        --md-description < ${NULL_DEVICE} > ${TMP_MD_FILE}
      COMMAND ${grass_env_command} ${PYTHON_EXECUTABLE} ${MKMARKDOWN_PY}
              ${G_TARGET_NAME} ${GRASS_VERSION_DATE} > ${OUT_MD_FILE}
      COMMAND ${copy_images_md_command}
      COMMAND ${CMAKE_COMMAND} -E remove ${TMP_MD_FILE}
              ${CMAKE_CURRENT_BINARY_DIR}/${G_TARGET_NAME}.md
      COMMAND ${grass_env_command} ${PYTHON_EXECUTABLE} ${MKMARKDOWN_PY}
              ${G_TARGET_NAME} ${GRASS_VERSION_DATE} > ${GUI_MD_FILE}
      COMMENT "Creating ${OUT_MD_FILE} and ${GUI_MD_FILE}"
      DEPENDS ${OUT_SCRIPT_FILE} GUI_WXPYTHON LIB_PYTHON)

    install(FILES ${OUT_HTML_FILE} ${GUI_HTML_FILE}
            DESTINATION ${GRASS_INSTALL_DOCDIR})
    install(FILES ${OUT_MD_FILE} ${GUI_MD_FILE}
            DESTINATION ${GRASS_INSTALL_MKDOCSDIR})

  endif() # WITH_DOCS

  add_custom_target(
    ${G_TARGET_NAME} DEPENDS ${GUI_STAMP_FILE} ${OUT_SCRIPT_FILE}
                             ${OUT_HTML_FILE} ${OUT_MD_FILE})

  set(modules_list
      "${G_TARGET_NAME};${modules_list}"
      CACHE INTERNAL "list of modules")

  set_target_properties(${G_TARGET_NAME} PROPERTIES FOLDER gui)

  if(WIN32)
    install(PROGRAMS ${OUTDIR}/${GRASS_INSTALL_SCRIPTDIR}/${G_TARGET_NAME}.bat
            DESTINATION ${GRASS_INSTALL_SCRIPTDIR})
  endif()

  install(
    DIRECTORY "${OUTDIR}/${GRASS_INSTALL_GUIDIR}/wxpython/${G_NAME}"
    DESTINATION "${GRASS_INSTALL_GUIDIR}/wxpython")

  install(
    PROGRAMS ${OUTDIR}/${GRASS_INSTALL_SCRIPTDIR}/${G_TARGET_NAME}${SCRIPT_EXT}
    DESTINATION ${GRASS_INSTALL_SCRIPTDIR})

endfunction()
