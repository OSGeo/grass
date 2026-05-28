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
      install(FILES ${IMG_FILES} DESTINATION ${GRASS_INSTALL_DOCDIR})
    endif()

    set(HTML_FILE ${G_SRC_DIR}/${G_TARGET_NAME}.html)
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

    set(out_html_file ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/${G_TARGET_NAME}.html)

    generate_docs(${G_NAME}
      OUTPUT ${out_html_file}
      GUI_TARGET_NAME ${G_TARGET_NAME}
      SOURCEDIR ${G_SRC_DIR}
      DEPENDS ${OUT_SCRIPT_FILE} GUI_WXPYTHON LIB_PYTHON)

  endif() # WITH_DOCS

  add_custom_target(
    ${G_TARGET_NAME} DEPENDS ${GUI_STAMP_FILE} ${OUT_SCRIPT_FILE}
                             ${out_html_file})

  set(modules_list
      "${G_TARGET_NAME};${modules_list}"
      CACHE INTERNAL "list of modules")

  set_target_properties(${G_TARGET_NAME} PROPERTIES FOLDER "Tools/GUI")

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
