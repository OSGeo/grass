

function(copy_python_file py_file install_dest)
  get_filename_component(py_file_NAME ${py_file} NAME)
  set(output_path ${GISBASE}/${install_dest})
  set(copied_file_path ${output_path}/${py_file_NAME})
   add_custom_command(OUTPUT ${copied_file_path}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${py_file} ${output_path}
	DEPENDS g.parser
    COMMENT "Copy ${output_path}/${py_file_NAME}"
    VERBATIM)
  set(pyc_file_path "${copied_file_path}c")
  add_custom_command(
    OUTPUT ${pyc_file_path}
    COMMAND ${PYTHON_EXECUTABLE} -t -m py_compile ${copied_file_path}
	DEPENDS ${copied_file_path}
    COMMENT "Generating ${pyc_file_path}"
    VERBATIM)

  #set_source_files_properties("${pyc_file_path}" GENERATED)
  string(REPLACE "/" "_" targ_name ${install_dest})
  #message(FATAL_ERROR "targ_name=${targ_name}")
  add_custom_target(${targ_name}_${py_file_NAME} ALL
    DEPENDS ${pyc_file_path})
SET_TARGET_PROPERTIES (${targ_name}_${py_file_NAME} PROPERTIES FOLDER copy)
  install(FILES ${copied_file_path} DESTINATION ${install_dest})

endfunction()
