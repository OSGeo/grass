# AUTHOR(S): Rashad Kanavath <rashad km gmail>
# PURPOSE: 	  A CMake function that builds grass python script modules
# COPYRIGHT: (C) 2020 by the GRASS Development Team
#   	    	 This program is free software under the GPL (>=v2)
#   	    	 Read the file COPYING that comes with GRASS for details.

function(build_script_in_subdir dir_name)
  #build_py_module(NAME ${dir_name})
  set(G_NAME ${dir_name})

  set(G_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${G_NAME})

  file(GLOB PYTHON_FILES "${G_SRC_DIR}/*.py")
  if(NOT PYTHON_FILES)
    message(FATAL_ERROR "[${G_NAME}]: No PYTHON_FILES found.")
  endif()

if(NOT PY_MODULE_FILE)
	set(PY_MODULE_FILE ${G_SRC_DIR}/${G_NAME}.py)
	if(EXISTS "${PY_MODULE_FILE}")
		file(COPY ${PY_MODULE_FILE} DESTINATION ${CMAKE_BINARY_DIR}/scripts/)
		set(MAIN_SCRIPT_FILE ${CMAKE_BINARY_DIR}/scripts/${G_NAME}.py)
	else()
	set(PY_MODULE_FILE "")
	set(MAIN_SCRIPT_FILE "")
	endif()
endif()

if (NOT EXISTS ${PY_MODULE_FILE})
	message(FATAL_ERROR "${PY_MODULE_FILE} does not exists")
endif()

 set(TRANSLATE_C_FILE
   ${CMAKE_SOURCE_DIR}/locale/scriptstrings/${G_NAME}_to_translate.c)  
if(WIN32)
    #file(INSTALL ${G_TARGET_FILE} DESTINATION ${OUTPUT_DIR} USE_SOURCE_PERMISSIONS)
	set(PGM_NAME ${G_NAME})
    configure_file(
	${CMAKE_SOURCE_DIR}/cmake/windows_launch.bat.in
	${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${G_NAME}.bat @ONLY)
	set(copy_script_command ${CMAKE_COMMAND} -E copy 
		${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${G_NAME}.bat 
		${GISBASE}/scripts/${G_NAME}.bat)
else(WIN32)
	set(copy_script_command ${CMAKE_COMMAND} -E echo "")
endif(WIN32)


add_custom_target(${G_NAME}
      DEPENDS g.parser ${PY_MODULE_FILE}
      COMMAND ${CMAKE_COMMAND}
      -DINPUT_FILE=${MAIN_SCRIPT_FILE}
      -DOUTPUT_FILE=${TRANSLATE_C_FILE}
      -DBIN_DIR=${CMAKE_BINARY_DIR}
      -P ${CMAKE_SOURCE_DIR}/cmake/locale_strings.cmake
	  COMMAND ${copy_script_command}
      VERBATIM)
  set(modules_list "${G_NAME};${modules_list}" CACHE INTERNAL "list of modules")

  set_target_properties (${G_NAME} PROPERTIES FOLDER scripts)

  install(PROGRAMS ${MAIN_SCRIPT_FILE} DESTINATION scripts)


    set(HTML_FILE_NAME ${G_NAME})
 if(WITH_DOCS)

 file(GLOB IMG_FILES ${G_SRC_DIR}/*.png  ${G_SRC_DIR}/*.jpg)
set(copy_images_command ${CMAKE_COMMAND} -E echo "")
if(IMG_FILES)
	set(copy_images_command ${CMAKE_COMMAND} -E copy ${IMG_FILES} ${GISBASE}/docs/html/)
endif()


  set(HTML_FILE ${G_SRC_DIR}/${G_NAME}.html)
    if(EXISTS ${HTML_FILE})
	install(FILES ${GISBASE}/docs/html/${G_NAME}.html DESTINATION docs/html)
  else()
	set(HTML_FILE)
    file(GLOB html_files ${G_SRC_DIR}/*.html)
    if(html_files)
		message(FATAL_ERROR "${html_file} does not exists. ${G_SRC_DIR} \n ${GISBASE}/scripts| ${G_NAME}")
    endif()
  endif()

set(TMP_HTML_FILE ${G_SRC_DIR}/${G_NAME}.tmp.html)
set(OUT_HTML_FILE ${GISBASE}/docs/html/${G_NAME}.html)


add_custom_command(TARGET ${G_NAME} POST_BUILD
  COMMAND ${grass_env_command} ${CMAKE_COMMAND} -E chdir ${G_SRC_DIR} 
  ${PYTHON_EXECUTABLE} ${G_NAME}.py "--html-description" > ${TMP_HTML_FILE}
  COMMAND ${grass_env_command} ${CMAKE_COMMAND} -E chdir ${G_SRC_DIR} 
  ${PYTHON_EXECUTABLE} ${MKHTML_PY} ${G_NAME} > ${OUT_HTML_FILE}
  COMMAND ${copy_images_command}
  COMMAND ${CMAKE_COMMAND} -E copy ${PY_MODULE_FILE} ${GISBASE}/scripts/
  COMMAND ${CMAKE_COMMAND} -E remove ${TMP_HTML_FILE}
  COMMENT "Creating ${OUT_HTML_FILE}")


endif(WITH_DOCS)

endfunction()