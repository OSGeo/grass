macro(build_gui_in_subdir dir_name)
  build_py_module(NAME ${dir_name}
    DST_DIR gui/wxpython
    TYPE "GUI")
endmacro()


macro(build_script_in_subdir dir_name)
  build_py_module(NAME ${dir_name}
    DST_DIR etc
    TYPE "SCRIPT")
endmacro()

macro(build_py_lib_in_subdir dir_name)
  build_py_module(NAME ${dir_name}
	TYPE "LIB"
    DST_DIR etc/python/grass)
endmacro()


function(build_py_module)
  cmake_parse_arguments(G
    ""
    "NAME;SRC_DIR;SRC_REGEX;DST_DIR;TYPE;HTML_FILE_NAME"
    "SOURCES;DEPENDS" ${ARGN} )

  if(NOT G_SRC_DIR)
    set(G_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${G_NAME})
  endif()

  if(NOT G_SRC_REGEX)
    set(G_SRC_REGEX "*.py")
  endif()

  if(NOT G_TYPE)
    message(FATAL_ERROR "TYPE argument is required")
  endif()

  set(types "GUI;LIB;SCRIPT")
  if(NOT "${G_TYPE}" IN_LIST types)
	message(FATAL_ERROR "TYPE is '${G_TYPE}'. Supported values are ${types}")
  endif()

  set(G_TARGET_NAME_PREFIX "")
  if(G_TYPE STREQUAL "GUI")
	set(G_TARGET_NAME_PREFIX "g.gui.")
  elseif(G_TYPE STREQUAL "LIB")
	set(G_TARGET_NAME_PREFIX "pylib.")
  endif()
  



  set(G_TARGET_NAME ${G_TARGET_NAME_PREFIX}${G_NAME})
  
  if(G_HTML_FILE_NAME)
    set(HTML_FILE_NAME ${G_HTML_FILE_NAME})
  else()
    set(HTML_FILE_NAME ${G_TARGET_NAME})
  endif()

  
  file(GLOB PYTHON_FILES "${G_SRC_DIR}/${G_SRC_REGEX}")
  if(NOT PYTHON_FILES)
    message(FATAL_ERROR "[${G_TARGET_NAME}]: No PYTHON_FILES found.")
  endif()



#  message("PYTHON_FILES=${PYTHON_FILES}")
  ##################### TRANSLATE STRING FOR SCRIPTS AND GUI #####################
  if(NOT PY_MODULE_FILE)
    set(PY_MODULE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${G_NAME}/${G_TARGET_NAME}.py)
  endif()

  if(NOT G_TYPE STREQUAL "LIB")
	if (NOT EXISTS ${PY_MODULE_FILE})
		message(FATAL_ERROR "${PY_MODULE_FILE} does not exists")
    endif()



  set(PGM_NAME ${G_TARGET_NAME})
  if(WIN32)
      set(PGM_NAME ${G_TARGET_NAME}.bat)      
      #file(TO_NATIVE_PATH "${GISBASE}/scripts" scripts_dir)
	  #file(TO_NATIVE_PATH "${PYTHON_EXECUTABLE}" python_exe)
	  #file(WRITE ${GISBASE}/scripts/${PGM_NAME} 
	  #"@echo off
#\"${python_exe}\" \"${scripts_dir}\\${G_TARGET_NAME}.py\" \%* ")
#message("Writing ${GISBASE}/scripts/${PGM_NAME} ")
      #configure_file(
	  #${CMAKE_SOURCE_DIR}/cmake/windows_launch.bat.in 
	  #${CMAKE_BINARY_DIR}/CMakeFiles/scripts/${PGM_NAME})
      ###file(COPY ${CMAKE_BINARY_DIR}/CMakeFiles/scripts/${PGM_NAME} DESTINATION ${CMAKE_BINARY_DIR}/scripts)
  else()
  	if(${G_TYPE} IN_LIST types)
	#configure_file(${PY_MODULE_FILE} ${CMAKE_BINARY_DIR}/CMakeFiles/scripts/${PGM_NAME})
	file(COPY ${PY_MODULE_FILE} DESTINATION ${CMAKE_BINARY_DIR}/TEMP/
	  FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)

	  	#COMMAND ${CMAKE_COMMAND} -E copy ${PY_MODULE_FILE} ${CMAKE_BINARY_DIR}//
	  endif()
  endif()
	  
    set(MAIN_SCRIPT_FILE "${GISBASE}/scripts/${PGM_NAME}")
	#message( "MAIN_SCRIPT_FILE=${MAIN_SCRIPT_FILE}")
	  endif()
  ######################## TRANSLATE STRING FOR SCRIPTS #########################
  set(TRANSLATE_C_FILE "")

  if(G_TYPE STREQUAL "SCRIPT")
    set(TRANSLATE_C_FILE
      ${CMAKE_SOURCE_DIR}/locale/scriptstrings/${G_NAME}_to_translate.c)  

    add_custom_command(
      OUTPUT ${TRANSLATE_C_FILE}
      DEPENDS g.parser ${PY_MODULE_FILE}
      COMMAND ${CMAKE_COMMAND}
      -DINPUT_FILE=${MAIN_SCRIPT_FILE}
      -DOUTPUT_FILE=${TRANSLATE_C_FILE}
      -DBIN_DIR=${CMAKE_BINARY_DIR}
      -P ${CMAKE_SOURCE_DIR}/cmake/locale_strings.cmake
      COMMENT "Generating ${TRANSLATE_C_FILE}"
      VERBATIM)
  endif()

 ## message("Adding python taret ${G_TARGET_NAME}")

  set(MAIN_SCRIPT_FILE ${CMAKE_BINARY_DIR}/TEMP/${G_TARGET_NAME}.py)
  if(EXISTS "${PY_MODULE_FILE}")
  file(COPY ${PY_MODULE_FILE} DESTINATION ${CMAKE_BINARY_DIR}/TEMP/)
  endif()
  add_custom_target(${G_TARGET_NAME} ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/${G_DST_DIR}/${G_NAME}/    
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PYTHON_FILES} ${GISBASE}/${G_DST_DIR}/${G_NAME}
    DEPENDS ${TRANSLATE_C_FILE} )

  #get_property(MODULE_LIST GLOBAL PROPERTY MODULE_LIST)
  #add_dependencies(${G_NAME} ${MODULE_LIST})

  if(G_DEPENDS)
    #add_dependencies(${G_TARGET_NAME} ${G_DEPENDS})
  endif()

  if(G_TYPE STREQUAL "GUI")
  set_target_properties (${G_TARGET_NAME} PROPERTIES FOLDER gui)
  endif()

  if(G_TYPE STREQUAL "SCRIPT")
  set_target_properties (${G_TARGET_NAME} PROPERTIES FOLDER scripts)
  endif()

  if(G_TYPE STREQUAL "LIB")
  set_target_properties (${G_TARGET_NAME} PROPERTIES FOLDER python)
  endif()

  install(PROGRAMS ${MAIN_SCRIPT_FILE} DESTINATION scripts)


 if(WITH_DOCS)
 	set(PGM_EXT "")
	if(NOT G_TYPE STREQUAL "LIB")
		if(WIN32)
			set(PGM_EXT ".bat")
		endif()




  set_target_properties(${G_TARGET_NAME} PROPERTIES G_SRC_DIR "${G_SRC_DIR}")
  set_target_properties(${G_TARGET_NAME} PROPERTIES G_TARGET_FILE "${MAIN_SCRIPT_FILE}")
  #set_target_properties(${G_TARGET_NAME} PROPERTIES PGM_NAME "${PGM_NAME}")
  set_target_properties(${G_TARGET_NAME} PROPERTIES RUN_HTML_DESCR TRUE)
  set_target_properties(${G_TARGET_NAME} PROPERTIES PYTHON_SCRIPT TRUE)
  #set_target_properties(${G_TARGET_NAME} PROPERTIES PGM_EXT "${PGM_EXT}")
  set_target_properties(${G_TARGET_NAME} PROPERTIES G_RUNTIME_OUTPUT_DIR "${GISBASE}/scripts")
  set_target_properties(${G_TARGET_NAME} PROPERTIES G_HTML_FILE_NAME "${HTML_FILE_NAME}.html")
  	if(${G_TYPE} IN_LIST types)
		build_docs(${G_TARGET_NAME})
		endif()
	endif()

	if(${G_TYPE} IN_LIST types)
		add_dependencies(${G_TARGET_NAME} pylib.script)
	endif()
 endif()

 install(FILES ${PYTHON_FILES} DESTINATION etc/${G_NAME})

endfunction()

