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


  set(SRC_SCRIPT_FILE ${G_SRC_DIR}/${G_NAME}.py)

  if (NOT EXISTS ${SRC_SCRIPT_FILE})
    message(FATAL_ERROR "${SRC_SCRIPT_FILE} does not exists")
    return()
  endif()


  set(SCRIPT_EXT "")
  if(WIN32)
    set(SCRIPT_EXT ".py")
  endif()

  set(TRANSLATE_C_FILE
    ${CMAKE_SOURCE_DIR}/locale/scriptstrings/${G_NAME}_to_translate.c)

  ADD_CUSTOM_COMMAND(OUTPUT ${TRANSLATE_C_FILE}
    COMMAND ${CMAKE_COMMAND}
    -DBINARY_DIR=${CMAKE_BINARY_DIR}
    -DG_NAME=${G_NAME}
    -DSRC_SCRIPT_FILE=${SRC_SCRIPT_FILE}
    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
    -DOUTPUT_FILE=${TRANSLATE_C_FILE}
    -P ${CMAKE_SOURCE_DIR}/cmake/locale_strings.cmake
    DEPENDS g.parser)

 set(HTML_FILE_NAME ${G_NAME})
 set(OUT_HTML_FILE "")

 if(WITH_DOCS)

   file(GLOB IMG_FILES ${G_SRC_DIR}/*.png  ${G_SRC_DIR}/*.jpg)
   set(copy_images_command ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${G_NAME})
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


   ADD_CUSTOM_COMMAND(OUTPUT ${OUT_HTML_FILE}
     COMMAND ${grass_env_command} ${CMAKE_COMMAND} -E chdir ${G_SRC_DIR}
     ${PYTHON_EXECUTABLE} ${G_NAME}.py "--html-description" > ${TMP_HTML_FILE}
     COMMAND ${grass_env_command} ${CMAKE_COMMAND} -E chdir ${G_SRC_DIR}
     ${PYTHON_EXECUTABLE} ${MKHTML_PY} ${G_NAME} > ${OUT_HTML_FILE}
     COMMAND ${copy_images_command}
     COMMAND ${CMAKE_COMMAND} -E remove ${TMP_HTML_FILE}
     COMMENT "Creating ${OUT_HTML_FILE}"
     DEPENDS ${TRANSLATE_C_FILE} LIB_PYTHON)

   install(FILES ${OUT_HTML_FILE} DESTINATION docs/html/)

 endif() #WITH_DOCS

 ADD_CUSTOM_TARGET(${G_NAME} DEPENDS ${TRANSLATE_C_FILE} ${OUT_HTML_FILE})

 set(modules_list "${G_NAME};${modules_list}" CACHE INTERNAL "list of modules")

 set_target_properties (${G_NAME} PROPERTIES FOLDER scripts)

 if(WIN32)
   install(PROGRAMS ${GISBASE}/scripts/${G_NAME}.bat DESTINATION scripts)
 endif()

 install(PROGRAMS ${GISBASE}/scripts/${G_NAME}${SCRIPT_EXT} DESTINATION scripts)

endfunction()
