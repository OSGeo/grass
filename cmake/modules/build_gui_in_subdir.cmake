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

  if(NOT PY_MODULE_FILE)
	set(PY_MODULE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${G_NAME}/${G_TARGET_NAME}.py)
	if(EXISTS "${PY_MODULE_FILE}")
		file(COPY ${PY_MODULE_FILE} DESTINATION ${CMAKE_BINARY_DIR}/scripts/)
		set(MAIN_SCRIPT_FILE ${CMAKE_BINARY_DIR}/scripts/${G_TARGET_NAME}.py)
	else()
		set(PY_MODULE_FILE "")
		set(MAIN_SCRIPT_FILE "")
	endif()
  endif()

  if (NOT EXISTS ${PY_MODULE_FILE})
	message(FATAL_ERROR "${PY_MODULE_FILE} does not exists")
  endif()

  if(WIN32)
    #file(INSTALL ${G_TARGET_FILE} DESTINATION ${OUTPUT_DIR} USE_SOURCE_PERMISSIONS)
	set(PGM_NAME ${G_TARGET_NAME})
    configure_file(
	${CMAKE_SOURCE_DIR}/cmake/windows_launch.bat.in
	${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${G_TARGET_NAME}.bat @ONLY)
	set(copy_script_command ${CMAKE_COMMAND} -E copy 
		${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${G_TARGET_NAME}.bat 
		${GISBASE}/scripts/${G_TARGET_NAME}.bat)
else(WIN32)
	set(copy_script_command ${CMAKE_COMMAND} -E echo "")
endif(WIN32)
  add_custom_target(${G_TARGET_NAME}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/gui/wxpython/${G_NAME}/
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PYTHON_FILES} ${GISBASE}/gui/wxpython/${G_NAME}/
		COMMAND ${copy_script_command}
		DEPENDS g.parser ${gui_lib_targets})

  set(modules_list "${G_TARGET_NAME};${modules_list}" CACHE INTERNAL "list of modules")

  set_target_properties (${G_TARGET_NAME} PROPERTIES FOLDER gui)

  install(PROGRAMS ${MAIN_SCRIPT_FILE} DESTINATION scripts)

  if(WITH_DOCS)

   file(GLOB IMG_FILES ${G_SRC_DIR}/*.png  ${G_SRC_DIR}/*.jpg)
set(copy_images_command ${CMAKE_COMMAND} -E echo "")
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

add_custom_command(TARGET ${G_TARGET_NAME} POST_BUILD
  COMMAND ${grass_env_command} ${CMAKE_COMMAND} -E chdir ${G_SRC_DIR} 
  ${PYTHON_EXECUTABLE} ${G_TARGET_NAME}.py "--html-description" > ${TMP_HTML_FILE}
  COMMAND ${grass_env_command} ${CMAKE_COMMAND} -E chdir ${G_SRC_DIR} 
  ${PYTHON_EXECUTABLE} ${MKHTML_PY} ${G_TARGET_NAME} > ${OUT_HTML_FILE}
  COMMAND ${copy_images_command}
  COMMAND ${CMAKE_COMMAND} -E copy ${PY_MODULE_FILE} ${GISBASE}/scripts/
  COMMAND ${CMAKE_COMMAND} -E remove ${TMP_HTML_FILE}
  COMMENT "Creating ${OUT_HTML_FILE}")

  endif(WITH_DOCS)
 ##install(FILES ${PYTHON_FILES} DESTINATION etc/${G_NAME})

endfunction()
