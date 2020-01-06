include(copy_python_file)
function(build_pymodule_in_subdir module_name dest_dir)
  #set(pyc_depends)

  file(GLOB img_files ${module_name}/*.png  ${module_name}/*.jpg)
  file(GLOB py_files ${module_name}/*.py)

  string(REPLACE "/" "_" target_name ${module_name})
  string(REPLACE "/" "_" targ_prefix ${dest_dir})
  set(targ_name ${targ_prefix}_${target_name} )

  set(g_gui_found FALSE)
  set(G_NAME g.gui.${module_name}) #Important as it is used in windows_launch.bat.in

  set(g_gui_file ${CMAKE_CURRENT_SOURCE_DIR}/${module_name}/${G_NAME}.py)
  ##message( "${GISBASE}/${dest_dir}")
  add_custom_target(py_${targ_name} ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/${dest_dir}/${module_name}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${py_files} ${GISBASE}/${dest_dir}/${module_name}
  DEPENDS g.parser
  )
  set_target_properties (py_${targ_name} PROPERTIES FOLDER ${dest_dir})
   if(img_files)
   add_custom_target(docs_images_${targ_name} ALL
   COMMAND ${CMAKE_COMMAND} -E copy_if_different ${img_files} ${GISBASE}/docs/html
   DEPENDS py_${targ_name})
   set_target_properties (docs_images_${targ_name} PROPERTIES FOLDER docs)
   endif()

  if(EXISTS ${g_gui_file})
  if(WIN32)
  configure_file(${CMAKE_SOURCE_DIR}/cmake/windows_launch.bat.in ${GISBASE}/bin/${G_NAME}.bat)
  install(PROGRAMS ${GISBASE}/bin/${G_NAME}.bat DESTINATION bin)
else()
  #  file(INSTALL ${g_gui_file} ${GISBASE}/scripts/${G_NAME}) # COPYONLY)

    configure_file(${g_gui_file} ${GISBASE}/scripts/${G_NAME} COPYONLY)

  install(PROGRAMS ${GISBASE}/scripts/${G_NAME} DESTINATION scripts)
  endif()

  if(WIN32)
    set(python_script ${GISBASE}/scripts/${G_NAME}.bat)
   else()
      add_custom_target(py_g_gui_${targ_name} ) # ALL
   #   COMMAND ${CMAKE_COMMAND} -E copy_if_different ${g_gui_file} ${GISBASE}/scripts/${G_NAME}
   #   DEPENDS py_${targ_name})
   # set_target_properties (py_g_gui_${targ_name} PROPERTIES FOLDER gui)
   set(python_script ${GISBASE}/scripts/${G_NAME})

  endif()

  install(FILES ${g_gui_file} DESTINATION scripts)



if(WITH_DOCS)
  set(html_file ${CMAKE_CURRENT_SOURCE_DIR}/${module_name}/${G_NAME}.html )

   set(tmp_html_cmd )
   set(mkhtml_cmd )
	
   set(html_file_tmp "${GISBASE}/docs/html/${G_NAME}.tmp.html")
   set(html_file_out "${GISBASE}/docs/html/${G_NAME}.html")
   set(html_file_gui "${GISBASE}/docs/html/wxGUI.${G_NAME}.html")
#message(FATAL_ERROR "py_g_gui_${targ_name}_html_descr")
   add_custom_target(py_g_gui_${targ_name}_html_descr
	  COMMAND ${RUN_PYTHON} ${python_script} --html-description > ${html_file_tmp}
	  DEPENDS py_g_gui_${targ_name}
   )

   add_custom_target(py_g_gui_${targ_name}_mkhtml
	  COMMAND ${RUN_PYTHON} ${GISBASE}/tools/mkhtml.py ${python_script} ${html_file} ${html_file_tmp} > ${html_file_out}
	  COMMAND ${CMAKE_COMMAND} -E remove ${html_file_tmp}
	  COMMAND ${RUN_PYTHON} ${GISBASE}/tools/mkhtml.py ${python_script} ${html_file} ${html_file_tmp} > ${html_file_gui} 
	  DEPENDS py_g_gui_${targ_name}_html_descr
   )

   add_custom_target(py_g_gui_${targ_name}_mkhtml_wxgui ALL
	  COMMAND ${RUN_PYTHON} ${GISBASE}/tools/mkhtml.py ${python_script} ${html_file} ${html_file_tmp} > ${html_file_gui} 
	  DEPENDS py_g_gui_${targ_name}_mkhtml
   )
endif() #WITH_DOCS
  endif()

  install(FILES ${py_files} DESTINATION ${dest_dir})
  if(WITH_DOCS)
  install(FILES ${img_files} DESTINATION docs/html)
  endif()

endfunction()
