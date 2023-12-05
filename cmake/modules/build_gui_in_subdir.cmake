# AUTHOR(S): Rashad Kanavath <rashad km gmail>
# PURPOSE: 	  build_gui_in_subdir is the cmake function that builds g.gui.* modules
# COPYRIGHT: (C) 2020 by the GRASS Development Team
#   	    	 This program is free software under the GPL (>=v2)
#   	    	 Read the file COPYING that comes with GRASS for details.

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

  if (NOT EXISTS ${SRC_SCRIPT_FILE})
    message(FATAL_ERROR "${SRC_SCRIPT_FILE} does not exists")
  endif()


  set(SCRIPT_EXT "")
  if(WIN32)
    set(SCRIPT_EXT ".py")
  endif()
  set(GUI_STAMP_FILE ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${G_NAME}.stamp)

  ADD_CUSTOM_COMMAND(OUTPUT ${GUI_STAMP_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/gui/wxpython/${G_NAME}/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PYTHON_FILES} ${GISBASE}/gui/wxpython/${G_NAME}/
    COMMAND ${CMAKE_COMMAND} -E touch ${GUI_STAMP_FILE})

  set(OUT_SCRIPT_FILE ${GISBASE}/scripts/${G_TARGET_NAME}${SCRIPT_EXT})
  ADD_CUSTOM_COMMAND(OUTPUT ${OUT_SCRIPT_FILE}
    COMMAND ${CMAKE_COMMAND}
    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
    -DBINARY_DIR=${CMAKE_BINARY_DIR}
    -DSRC_SCRIPT_FILE=${SRC_SCRIPT_FILE}
    -DG_NAME=${G_TARGET_NAME}
    -DGISBASE=${GISBASE}
    -P ${CMAKE_SOURCE_DIR}/cmake/copy_g_gui_module.cmake
    DEPENDS g.parser ${SRC_SCRIPT_FILE})


  if(WITH_DOCS)

    file(GLOB IMG_FILES ${G_SRC_DIR}/*.png  ${G_SRC_DIR}/*.jpg)
    set(copy_images_command ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${G_TARGET_NAME})
    if(IMG_FILES)
      set(copy_images_command ${CMAKE_COMMAND} -E copy ${IMG_FILES} ${GISBASE}/docs/html/)
    endif()

    set(HTML_FILE ${G_SRC_DIR}/${G_TARGET_NAME}.html)
    if(EXISTS ${HTML_FILE})
      install(FILES ${GISBASE}/docs/html/${G_TARGET_NAME}.html DESTINATION docs/html)
    else()
      set(HTML_FILE)
      file(GLOB html_files ${G_SRC_DIR}/*.html)
      if(html_files)
	message(FATAL_ERROR "${html_file} does not exists. ${G_SRC_DIR} \n ${GISBASE}/scripts| ${G_TARGET_NAME}")
      endif()
    endif()

    set(TMP_HTML_FILE ${G_SRC_DIR}/${G_TARGET_NAME}.tmp.html)
    set(OUT_HTML_FILE ${GISBASE}/docs/html/${G_TARGET_NAME}.html)
    set(GUI_HTML_FILE ${GISBASE}/docs/html/wxGUI.${G_NAME}.html)

  ADD_CUSTOM_COMMAND(OUTPUT ${OUT_HTML_FILE}
    COMMAND ${grass_env_command} ${CMAKE_COMMAND} -E chdir ${G_SRC_DIR}
    ${PYTHON_EXECUTABLE} ${G_TARGET_NAME}.py "--html-description" > ${TMP_HTML_FILE}
    COMMAND ${grass_env_command} ${CMAKE_COMMAND} -E chdir ${G_SRC_DIR}
    ${PYTHON_EXECUTABLE} ${MKHTML_PY} ${G_TARGET_NAME} ${GRASS_VERSION_DATE} > ${OUT_HTML_FILE}
    COMMENT "Creating ${OUT_HTML_FILE}"
    COMMAND ${copy_images_command}
    COMMAND ${CMAKE_COMMAND} -E remove ${TMP_HTML_FILE}
    COMMAND ${grass_env_command} ${CMAKE_COMMAND} -E chdir ${G_SRC_DIR}
    ${PYTHON_EXECUTABLE} ${MKHTML_PY} ${G_TARGET_NAME} ${GRASS_VERSION_DATE} > ${GUI_HTML_FILE}
    COMMENT "Creating ${GUI_HTML_FILE}"
    DEPENDS ${OUT_SCRIPT_FILE} GUI_WXPYTHON LIB_PYTHON
    )

  install(FILES ${OUT_HTML_FILE} DESTINATION docs/html/)

  endif() #WITH_DOCS


  ADD_CUSTOM_TARGET(${G_TARGET_NAME}
    DEPENDS ${GUI_STAMP_FILE} ${OUT_SCRIPT_FILE} ${OUT_HTML_FILE})


  set(modules_list "${G_TARGET_NAME};${modules_list}" CACHE INTERNAL "list of modules")

  set_target_properties (${G_TARGET_NAME} PROPERTIES FOLDER gui)


  if(WIN32)
    install(PROGRAMS ${GISBASE}/scripts/${G_TARGET_NAME}.bat DESTINATION scripts)
  endif()

  install(PROGRAMS ${GISBASE}/scripts/${G_TARGET_NAME}${SCRIPT_EXT} DESTINATION scripts)


endfunction()
