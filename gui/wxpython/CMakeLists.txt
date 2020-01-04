

include(copy_python_file)
function(build_gui_module_in_dir module_name)

file(GLOB py_files ${module_name}/*.py)
file(GLOB img_files ${module_name}/*.png  ${module_name}/*.jpg )

set(g_gui_found FALSE)
foreach(py_file ${py_files})
if(py_file MATCHES "g.gui.*")
set(g_gui_found TRUE)
copy_python_file(${py_file} scripts)
endif()
copy_python_file(${py_file} gui/wxpython/${module_name})
endforeach()

foreach(img_file ${img_files})
file(COPY ${img_file} DESTINATION ${CMAKE_BINARY_DIR}/docs/html/)
endforeach()
if(g_gui_found)
if(WIN32)
set(python_script "g.gui.${module_name}.py")
else()
set(python_script "g.gui.${module_name}")
endif()
set(gui_module_name "g.gui.${module_name}")

set(html_file_path ${CMAKE_CURRENT_SOURCE_DIR}/${module_name}/g.gui.${module_name}.html )
#message(FATAL_ERROR "html_file_path=${html_file_path}")
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/docs/html/${gui_module_name}.tmp.html
	COMMAND ${CMAKE_BINARY_DIR}/tools/run_python.bat
	${CMAKE_BINARY_DIR}/scripts/${python_script}
	--html-description > ${CMAKE_BINARY_DIR}/docs/html/${gui_module_name}.tmp.html
	DEPENDS ${CMAKE_BINARY_DIR}/scripts/${python_script}
	COMMENT "Generating ${gui_module_name}.tmp.html"
	VERBATIM)


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

    add_custom_target(g_gui_${module_name}_docs ALL
    DEPENDS
	${CMAKE_BINARY_DIR}/docs/html/${gui_module_name}.html
	${CMAKE_BINARY_DIR}/docs/html/wxGUI.${module_name}.html)

	set_target_properties (g_gui_${module_name}_docs PROPERTIES FOLDER docs)
endif() #g_gui_found
endfunction()
#missing docs
set(gui_sub_dirs animation core datacatalog dbmgr gcp gmodeler gui_core iclass
icons image2target iscatt lmgr location_wizard mapdisp mapswipe mapwin modules nviz
photo2image psmap rdigit rlisetup startup timeline tools tplot vdigit
vnet web_services wxplot)

foreach(gui_sub_dir ${gui_sub_dirs})
  build_gui_module_in_dir(${gui_sub_dir})
endforeach()
copy_python_file(${CMAKE_CURRENT_SOURCE_DIR}/gis_set.py gui/wxpython)
copy_python_file(${CMAKE_CURRENT_SOURCE_DIR}/gis_set_error.py gui/wxpython)
copy_python_file(${CMAKE_CURRENT_SOURCE_DIR}/wxgui.py gui/wxpython)

install(FILES README DESTINATION gui/wxpython)
