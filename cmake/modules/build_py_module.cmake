macro(build_gui_in_subdir dir_name)
  build_py_module(NAME ${dir_name}
    DST_DIR gui/wxpython
    TARGET_NAME_PREFIX "g.gui."
    AS_GUI)
endmacro()


macro(build_script_in_subdir dir_name)
  build_py_module(NAME ${dir_name}
    DST_DIR etc
    AS_SCRIPT)
endmacro()

macro(build_py_lib_in_subdir dir_name)
  build_py_module(NAME ${dir_name}
    TARGET_NAME_PREFIX "pylib."
    DST_DIR etc/python/grass)
endmacro()


function(build_py_module)
  cmake_parse_arguments(G
    "AS_SCRIPT;AS_GUI"
    "NAME;SRC_DIR;SRC_REGEX;DST_DIR;TARGET_NAME_PREFIX;HTML_FILE_NAME"
    "SOURCES;DEPENDS" ${ARGN} )

  if(NOT G_SRC_DIR)
    set(G_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${G_NAME})
  endif()

  if(NOT G_SRC_REGEX)
    set(G_SRC_REGEX "*.py")
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
    set(PY_MODULE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${G_NAME}/${G_NAME}.py)
  endif()

  if(EXISTS ${PY_MODULE_FILE})
    set(MAIN_SCRIPT_FILE "${CMAKE_CURRENT_BINARY_DIR}/${G_NAME}")
    if(WIN32)
      configure_file(
	${CMAKE_SOURCE_DIR}/cmake/windows_launch.bat.in
	${MAIN_SCRIPT_FILE}.bat)
      #From here .bat is our main script file
      set(MAIN_SCRIPT_FILE ${MAIN_SCRIPT_FILE}.bat)
    else()
      if(G_TARGET_NAME STREQUAL "v.what.strds")
	##message(FATAL_ERROR "stop  MAIN_SCRIPT_FILE=${MAIN_SCRIPT_FILE}")
 endif()

      configure_file(${PY_MODULE_FILE} ${MAIN_SCRIPT_FILE} COPYONLY)
    endif()
  else()
    set(PY_MODULE_FILE "")
  endif()

   if(G_TARGET_NAME STREQUAL "v.what.strds")
    ###message(FATAL_ERROR "stop  PY_MODULE_FILE=${PY_MODULE_FILE} \nMAIN_SCRIPT_FILE=${MAIN_SCRIPT_FILE}")
 endif()
  ######################## TRANSLATE STRING FOR SCRIPTS #########################
  set(TRANSLATE_C_FILE "")

  if(G_AS_SCRIPT)
    set(TRANSLATE_C_FILE
      ${CMAKE_SOURCE_DIR}/locale/scriptstrings/${G_NAME}_to_translate.c)  

    add_custom_command(
      OUTPUT ${TRANSLATE_C_FILE}
      DEPENDS g.parser ${MAIN_SCRIPT_FILE}
      COMMAND ${CMAKE_COMMAND}
      -DINPUT_FILE=${MAIN_SCRIPT_FILE}
      -DOUTPUT_FILE=${TRANSLATE_C_FILE}
      -DBIN_DIR=${CMAKE_BINARY_DIR}
      -P ${CMAKE_SOURCE_DIR}/cmake/locale_strings.cmake
      COMMENT "Generating ${TRANSLATE_C_FILE}"
      VERBATIM)
  endif()

  #message("Adding python taret ${G_TARGET_NAME}")
  
  add_custom_target(${G_TARGET_NAME} ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/${G_DST_DIR}/${G_NAME}/    
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PYTHON_FILES} ${GISBASE}/${G_DST_DIR}/${G_NAME}
    DEPENDS ${TRANSLATE_C_FILE})

  #get_property(MODULE_LIST GLOBAL PROPERTY MODULE_LIST)
  #add_dependencies(${G_NAME} ${MODULE_LIST})

  if(G_DEPENDS)
    add_dependencies(${G_TARGET_NAME} ${G_DEPENDS})
  endif()
  set_target_properties (${G_TARGET_NAME} PROPERTIES FOLDER scripts)

  install(PROGRAMS ${MAIN_SCRIPT_FILE} DESTINATION scripts)

  # To use this property later in build_docs
  set(RUN_HTML_DESCR FALSE)
  if(G_AS_SCRIPT OR G_AS_GUI)
    set(RUN_HTML_DESCR TRUE)
  endif()

  set_target_properties(${G_TARGET_NAME} PROPERTIES G_SRC_DIR "${G_SRC_DIR}")
  set_target_properties(${G_TARGET_NAME} PROPERTIES G_PGM "${MAIN_SCRIPT_FILE}") 
  set_target_properties(${G_TARGET_NAME} PROPERTIES RUN_HTML_DESCR "${RUN_HTML_DESCR}")
  set_target_properties(${G_TARGET_NAME} PROPERTIES G_RUNTIME_OUTPUT_DIR "${GISBASE}/scripts") 
  set_target_properties(${G_TARGET_NAME} PROPERTIES G_HTML_FILE_NAME "${HTML_FILE_NAME}.html")
  set_target_properties(${G_TARGET_NAME} PROPERTIES IS_PYTHON_SCRIPT TRUE)

 if(WITH_DOCS)
	build_docs(${G_TARGET_NAME})
	add_dependencies(${G_TARGET_NAME} pylib.grass)
 endif()

 install(FILES ${PYTHON_FILES} DESTINATION etc/${G_NAME})

endfunction()

