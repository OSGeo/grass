
# missing docs


set(WXPYTHON_DIR ${GISBASE}/gui/wxpython)
set(gui_lib_DIRS core gui_core icons iscatt lmgr location_wizard
mapdisp mapwin modules nviz rdigit startup tools vnet web_services wxplot)

set(gui_lib_targets)
foreach(gui_lib_DIR ${gui_lib_DIRS})
  copy_python_files_in_subdir(${gui_lib_DIR} gui/wxpython)
  list(APPEND gui_lib_targets python_${gui_lib_DIR})
endforeach()
add_custom_target(GUI_WXPYTHON
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/wxgui.py ${WXPYTHON_DIR}/
	DEPENDS ${gui_lib_targets})

set(gui_DIRS animation datacatalog dbmgr gcp gmodeler
iclass image2target mapswipe photo2image psmap rlisetup timeline tplot vdigit)
set(g_gui_targets)
foreach(gui_DIR ${gui_DIRS})
  build_gui_in_subdir(${gui_DIR})
  list(APPEND g_gui_targets "g.gui.${gui_DIR}")
endforeach()


add_subdirectory(docs)

install(FILES README DESTINATION gui/wxpython)

  ### copy all python files gui/ lib/python
  ### compile all python files
  ### so below target depends on MODULD_LIST

add_custom_target(copy_wxpython_xml
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/xml  ${WXPYTHON_DIR}/xml/
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../xml/grass-interface.dtd ${GISBASE}/gui/
	DEPENDS GUI_WXPYTHON)

add_custom_target(compile_python_files
	COMMAND ${PYTHON_EXECUTABLE} -m compileall ${GISBASE}/scripts
	COMMAND ${PYTHON_EXECUTABLE} -m compileall ${WXPYTHON_DIR}
	DEPENDS ALL_MODULES)

add_custom_target(build_modules_items_xml
	COMMAND ${grass_env_command} ${PYTHON_EXECUTABLE}
	${CMAKE_CURRENT_SOURCE_DIR}/tools/build_modules_xml.py > ${WXPYTHON_DIR}/xml/module_items.xml
	DEPENDS copy_wxpython_xml compile_python_files ${g_gui_targets}
	COMMENT "Generating interface description for all modules..."
	VERBATIM)

add_custom_target(build_xml_menudata
   COMMAND ${grass_env_command} ${PYTHON_EXECUTABLE}
   ${CMAKE_CURRENT_SOURCE_DIR}/core/toolboxes.py > ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/menudata.xml
   COMMAND ${grass_env_command}  ${PYTHON_EXECUTABLE}
   ${CMAKE_CURRENT_SOURCE_DIR}/core/toolboxes.py "validate" ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/menudata.xml
   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/menudata.xml ${WXPYTHON_DIR}/xml/
   DEPENDS build_modules_items_xml)

add_custom_target(build_module_tree_menudata
  COMMAND ${grass_env_command} ${PYTHON_EXECUTABLE}
   ${CMAKE_CURRENT_SOURCE_DIR}/core/toolboxes.py "module_tree" > ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/module_tree_menudata.xml
  COMMAND ${grass_env_command} ${PYTHON_EXECUTABLE}
   ${CMAKE_CURRENT_SOURCE_DIR}/core/toolboxes.py "validate" ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/module_tree_menudata.xml
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/module_tree_menudata.xml ${WXPYTHON_DIR}/xml/
  DEPENDS build_xml_menudata)

add_custom_target(build_menustrings ALL
  COMMAND ${grass_env_command} ${PYTHON_EXECUTABLE}
   ${CMAKE_CURRENT_SOURCE_DIR}/core/menutree.py "manager" >> ${CMAKE_CURRENT_SOURCE_DIR}/menustrings.py
  COMMAND ${grass_env_command} ${PYTHON_EXECUTABLE}
   ${CMAKE_CURRENT_SOURCE_DIR}/core/menutree.py "module_tree" >> ${CMAKE_CURRENT_SOURCE_DIR}/menustrings.py
  COMMAND ${grass_env_command} ${PYTHON_EXECUTABLE}
   ${CMAKE_CURRENT_SOURCE_DIR}/core/menutree.py "psmap" >> ${CMAKE_CURRENT_SOURCE_DIR}/menustrings.py
  DEPENDS build_module_tree_menudata gui_images)
