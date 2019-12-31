function(copy_python_file py_file py_module_name py_DIR_PATH install_dest)
  # A single case where setup.py from cmake binary directory is added. 
  # For this corner case we need to check for IS_ABSOLUTE first!s
  if(NOT IS_ABSOLUTE ${py_file})
    get_filename_component(py_file_REALPATH ${py_DIR_PATH}/${py_file} REALPATH)
  else()
	set(py_file_REALPATH ${py_file})
  endif()

  get_filename_component(py_file_NAME ${py_file_REALPATH} NAME)
	 
  set(output_path ${CMAKE_CURRENT_BINARY_DIR}/${py_DIR_PATH})
  file(COPY ${py_file_REALPATH} DESTINATION ${output_path})
  set(copied_py_file ${output_path}/${py_file_NAME})
  #rkm: TEMPORARY
  if(NOT EXISTS ${copied_py_file})
    message(FATAL_ERROR "not exists temp ${output_path};;${py_file_NAME} ")
  endif()

  set(stamp_file_path ${output_path}/CMakeFiles/${py_file_NAME}.stamp)
  add_custom_command(
    OUTPUT ${stamp_file_path}
    COMMAND ${PYTHON_EXECUTABLE} -t -m py_compile ${copied_py_file}
    COMMAND ${CMAKE_COMMAND} -E touch  ${stamp_file_path}
    COMMENT "Generating ${copied_py_file}c"
    VERBATIM)
  
  add_custom_target(${py_module_name}.${py_file_NAME} ALL
    DEPENDS ${stamp_file_path})
    
  set_source_files_properties("${copied_py_file}c" GENERATED)
  install(FILES ${copied_py_file} DESTINATION ${install_dest})

endfunction()