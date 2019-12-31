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
  set(copied_file_path ${output_path}/${py_file_NAME})

  set(pyc_file_path "${copied_file_path}c")
  add_custom_command(
    OUTPUT ${pyc_file_path}
    COMMAND ${PYTHON_EXECUTABLE} -t -m py_compile ${copied_file_path}
    COMMENT "Generating ${pyc_file_path}"
    VERBATIM)

  set_source_files_properties("${pyc_file_path}" GENERATED)

  add_custom_target(${py_module_name}.${py_file_NAME} ALL
    DEPENDS ${pyc_file_path})

  install(FILES ${copied_file_path} DESTINATION ${install_dest})

endfunction()