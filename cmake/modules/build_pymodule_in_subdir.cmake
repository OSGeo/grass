include(copy_python_file)
function(build_pymodule_in_subdir module_name dest_dir)
  #set(pyc_depends)

  file(GLOB img_files ${module_name}/*.png  ${module_name}/*.jpg)
  file(GLOB py_files ${module_name}/*.py)

  string(REPLACE "/" "_" target_name ${module_name})
  string(REPLACE "/" "_" targ_prefix ${dest_dir})
  set(targ_name ${targ_prefix}_${target_name} )

  set(g_gui_found FALSE)
  file(GLOB g_gui_files ${module_name}/g.gui.*.py)

  add_custom_target(py_${targ_name} 
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/${dest_dir}/${module_name}
  COMMAND ${CMAKE_COMMAND} -E copy ${py_files} ${CMAKE_BINARY_DIR}/${dest_dir}/${module_name}
  COMMAND ${PYTHON_EXECUTABLE} -m compileall  ${CMAKE_BINARY_DIR}/${dest_dir}/${module_name}
  DEPENDS g.parser
  )
  set_target_properties (py_${targ_name} PROPERTIES FOLDER ${dest_dir})
   if(img_files)
   add_custom_target(docs_images_${targ_name} 
   COMMAND ${CMAKE_COMMAND} -E copy ${img_files} ${CMAKE_BINARY_DIR}/docs/html)
   set_target_properties (docs_images_${targ_name} PROPERTIES FOLDER docs)
   endif()

  if(g_gui_files)
   add_custom_target(py_g_gui_${targ_name} 
	COMMAND ${CMAKE_COMMAND} -E copy ${g_gui_files} ${CMAKE_BINARY_DIR}/scripts
	COMMAND ${PYTHON_EXECUTABLE} -m compileall  ${CMAKE_BINARY_DIR}/scripts
	)
	 set_target_properties (py_g_gui_${targ_name} PROPERTIES FOLDER gui)
	install(FILES ${py_files} DESTINATION scripts)

	if(WIN32)
set(python_script "g.gui.${module_name}.py")
else()
set(python_script "g.gui.${module_name}")
endif()
set(gui_module_name "g.gui.${module_name}")


	
set(html_file ${CMAKE_CURRENT_SOURCE_DIR}/${module_name}/${gui_module_name}.html )
#message(FATAL_ERROR "html_file_path=${html_file_path}")

	set(tmp_html_cmd ${CMAKE_BINARY_DIR}/tools/run_grass.bat ${python_script} --html-description)
	set(mkhtml_cmd ${CMAKE_BINARY_DIR}/tools/run_python.bat ${CMAKE_BINARY_DIR}/tools/mkhtml.py)
	
	set(html_file_tmp "${CMAKE_BINARY_DIR}/docs/html/${gui_module_name}.tmp.html")
	set(html_file_out "${CMAKE_BINARY_DIR}/docs/html/${gui_module_name}.html")
	set(html_file_gui  "${CMAKE_BINARY_DIR}/docs/html/wxGUI.${module_name}.html")
	  ADD_CUSTOM_COMMAND(TARGET py_g_gui_${targ_name} POST_BUILD
	  COMMAND ${tmp_html_cmd} > ${html_file_tmp}
	  COMMAND ${mkhtml_cmd} ${python_script} ${html_file} ${html_file_tmp} > ${html_file_out}
	  COMMAND ${CMAKE_COMMAND} -E remove ${html_file_tmp}
	 COMMAND ${mkhtml_cmd} ${gui_module_name} ${html_file} > ${html_file_gui} 
	  )

  endif()

  #add_custom_target(py_compile_all_${targ_name}
  #COMMAND ${PYTHON_EXECUTABLBE} -m compileall  ${CMAKE_BINARY_DIR}/${dest_dir}
  #COMMAND ${PYTHON_EXECUTABLBE} -m compileall  ${CMAKE_BINARY_DIR}/scripts

  install(FILES ${py_files} DESTINATION ${dest_dir})
  install(FILES ${img_files} DESTINATION docs/html)

  #set(last_depends_file)
  #foreach(py_file ${py_files})
  #  get_filename_component(py_file_NAME ${py_file} NAME)
#	set(output_path ${CMAKE_BINARY_DIR}/${dest_dir})
#	set(copied_file_path ${output_path}/${py_file_NAME})
   #add_custom_command(OUTPUT ${copied_file_path}
#    COMMAND ${CMAKE_COMMAND} -E copy ${py_file} ${output_path}
	#DEPENDS g.parser
    #COMMENT "Copied ${copied_file_path}"
    #VERBATIM)
  #set(last_depends_file "${copied_file_path}c")
  #add_custom_command(
   # OUTPUT ${last_depends_file}
#    COMMAND ${PYTHON_EXECUTABLE} -t -m py_compile ${copied_file_path}
#	DEPENDS ${copied_file_path}
#    COMMENT "Generating ${last_depends_file}"
    #VERBATIM)





	#install(FILES ${py_file} DESTINATION ${dest_dir})


     #copy_python_file(${py_file} ${dest_dir}/${module_name})
	 #list(APPEND pyc_depends ${output_pyc_file})
  #endforeach()
  
  #string(REPLACE "/" "_" targ_name ${dest_dir})
  #add_custom_target(py_${module} DEPENDS ${pyc_depends})

  #file(GLOB img_files ${module_name}/*.png  ${module_name}/*.jpg)
  #foreach(img_file ${img_files})
#	get_filename_component(img_file_NAME ${img_file} NAME)
#	set(copied_file_path ${CMAKE_BINARY_DIR}/docs/html/${img_file_NAME})
	#add_custom_command(OUTPUT ${copied_file_path}
    #COMMAND ${CMAKE_COMMAND} -E copy ${img_file} ${CMAKE_BINARY_DIR}/docs/html
	#DEPENDS ${last_depends_file}
    #COMMENT "Copied ${copied_file_path}"
    #VERBATIM)
	#	install(FILES ${img_file} DESTINATION ${dest_dir})
  #endforeach()



endfunction()







add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/docs/html/${gui_module_name}.html
	COMMAND ${CMAKE_BINARY_DIR}/tools/run_python.bat
	${CMAKE_BINARY_DIR}/tools/mkhtml.py ${gui_module_name}
	${html_file_path} > ${CMAKE_BINARY_DIR}/docs/html/${gui_module_name}.html
	COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/docs/html/${gui_module_name}.tmp.html
	DEPENDS ${CMAKE_BINARY_DIR}/docs/html/${gui_module_name}.tmp.html
	COMMENT "Generating docs/html/${gui_module_name}.html"
  VERBATIM)

  add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/docs/html/wxGUI.${module_name}.html
	COMMAND ${CMAKE_BINARY_DIR}/tools/run_python.bat
	${CMAKE_BINARY_DIR}/tools/mkhtml.py  ${gui_module_name}
	${html_file_path} > ${CMAKE_BINARY_DIR}/docs/html/wxGUI.${module_name}.html
	COMMENT "Generating docs/html/wxGUI.${module_name}.html"
  VERBATIM)
