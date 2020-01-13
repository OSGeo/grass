function(build_docs target_name)
  get_target_property(G_SRC_DIR ${target_name} G_SRC_DIR)
  get_target_property(G_TARGET_FILE ${target_name} G_TARGET_FILE)
  get_target_property(RUN_HTML_DESCR ${target_name} RUN_HTML_DESCR)
  get_target_property(G_RUNTIME_OUTPUT_DIR ${target_name} G_RUNTIME_OUTPUT_DIR)
  get_target_property(G_HTML_FILE_NAME ${target_name} G_HTML_FILE_NAME)
  get_target_property(PYTHON_SCRIPT ${target_name} PYTHON_SCRIPT)


  set(html_file ${G_SRC_DIR}/${G_HTML_FILE_NAME})
  set(HTML_FILE)
  set(no_docs_list "grass_sqlp;echo;clean_temp;lock;run")
  
  if(EXISTS ${html_file})
    set(HTML_FILE ${html_file})
	install(FILES ${GISBASE}/docs/html/${G_HTML_FILE_NAME} DESTINATION docs/html)
  else()
    file(GLOB html_files ${G_SRC_DIR}/*.html)
    if(html_files)
      if(NOT ${target_name} IN_LIST no_docs_list)
		message(FATAL_ERROR "${html_file} does not exists. ${G_SRC_DIR} \n ${G_RUNTIME_OUTPUT_DIR} | ${target_name}")
      endif()
    endif()
  endif()
  

  add_custom_command(TARGET ${target_name} POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    -DHTML_FILE=${HTML_FILE}
    -DRUN_HTML_DESCR=${RUN_HTML_DESCR}
	-DG_TARGET_FILE=${G_TARGET_FILE}
    -DOUTPUT_DIR=${G_RUNTIME_OUTPUT_DIR}
    -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE}
	-DPYTHON_SCRIPT=${PYTHON_SCRIPT}
    -P ${CMAKE_BINARY_DIR}/cmake/mkhtml.cmake
    )
  
  #add_custom_target(${target_name}_html ALL DEPENDS ${G_RUNTIME_OUTPUT_DIR}/${target_name}.exe)


 endfunction()
