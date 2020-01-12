
# missing docs
set(gui_DIRS animation datacatalog dbmgr gcp gmodeler 
iclass image2target mapswipe photo2image psmap rlisetup timeline tplot vdigit)

foreach(gui_DIR ${gui_DIRS})
  build_gui_in_subdir(${gui_DIR})
endforeach()

set(WXPYTHON_DIR ${GISBASE}/gui/wxpython)
set(gui_lib_DIRS core gui_core icons iscatt lmgr location_wizard 
mapdisp mapwin modules nviz rdigit startup tools vnet web_services wxplot)

add_custom_target(gui_wxpython
	#COMMAND ${CMAKE_COMMAND} -E echo "#coding=utf-8" > ${WXPYTHON_DIR}/__initx__.py
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/gis_set.py ${WXPYTHON_DIR}/
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/gis_set_error.py ${WXPYTHON_DIR}/
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/wxgui.py ${WXPYTHON_DIR}/
	DEPENDS python_core python_gui_core python_icons python_iscatt python_lmgr python_location_wizard 
	python_mapdisp python_mapwin python_modules python_nviz python_rdigit python_startup python_vnet
	python_web_services python_wxplot)

foreach(gui_lib_DIR ${gui_lib_DIRS})
  copy_python_files_in_subdir(${gui_lib_DIR} gui/wxpython)
endforeach()

install(FILES README DESTINATION gui/wxpython)


#get_property(MODULE_LIST GLOBAL PROPERTY MODULE_LIST)
#separate_arguments(MODULE_LIST)
#set(EXCLUDED_MODULE_LIST "current_time_s_ms;g.parser;cell;vector;v.in.pdal;i.find")
#list(REMOVE_ITEM MODULE_LIST ${EXCLUDED_MODULE_LIST})
#list(REMOVE_DUPLICATES MODULE_LIST)

set(XXX 1)
if(XXX)
  ### copy all python files gui/ lib/python
  ### compile all python files
  ### so below target depends on MODULD_LIST

add_custom_target(copy_wxpython_xml 
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/xml  ${WXPYTHON_DIR}/xml/
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../xml/grass-interface.dtd ${GISBASE}/gui/
#####  DEPENDS copy_gui_images
DEPENDS ALL_MODULES
  )
add_custom_target(compile_python_files 
  COMMAND ${PYTHON_EXECUTABLE} -m compileall ${GISBASE}/scripts
  COMMAND ${PYTHON_EXECUTABLE} -m compileall ${WXPYTHON_DIR}
  DEPENDS copy_wxpython_xml)

file(TO_NATIVE_PATH "${GISBASE}" GISBASE_NATIVE)
file(TO_NATIVE_PATH "${CMAKE_BINARY_DIR}/bin" BINARY_DIR)
file(TO_NATIVE_PATH "${CMAKE_BINARY_DIR}/lib" LIB_DIR)
file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}" MODULE_TOPDIR)
file(TO_NATIVE_PATH "${GISBASE}/scripts" SCRIPTS_DIR)
file(TO_NATIVE_PATH "${GISBASE}/etc/config/rc" GISRC)
file(TO_NATIVE_PATH "${GISBASE}/etc/python" ETC_PYTHON_DIR)
file(TO_NATIVE_PATH "${GISBASE}/gui/wxpython" ETC_WXPYTHON_DIR)
if(WIN32)
set(sep "\;")
else()
set(sep "\:")
endif()

set(env_command ${CMAKE_COMMAND} -E env 
	"PATH=${BINARY_DIR}${sep}${SCRIPTS_DIR}"
	"PYTHONPATH=${ETC_PYTHON_DIR}${sep}${ETC_WXPYTHON_DIR}${sep}$ENV{PYTHONPATH}"
	"LD_LIBRARY_PATH=${LIB_DIR}${sep}$ENV{LD_LIBRARY_PATH}"
	"GISBASE=${GISBASE_NATIVE}"
	"GISRC=${GISRC}"
	"LC_ALL=C" 
	"LANG=C"
	"LANGUAGE=C"
	"MODULE_TOPDIR=${MODULE_TOPDIR}"
	"GRASS_PYTHON=${PYTHON_EXECUTABLE}"
    "VERSION_NUMBER=\"7.9.dev\""
    "VERSION_DATE=\"2019\"")
  add_custom_target(build_modules_items_xml 
	COMMAND ${env_command} ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_modules_xml.py > ${WXPYTHON_DIR}/xml/module_items.xml
  DEPENDS compile_python_files
  COMMENT "Generating interface description for all modules..."
  VERBATIM)

   add_custom_target(build_xml_menudata
   COMMAND ${env_command} ${PYTHON_EXECUTABLE} 
   ${CMAKE_CURRENT_SOURCE_DIR}/core/toolboxes.py > ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/menudata.xml
   COMMAND ${env_command}  ${PYTHON_EXECUTABLE} 
   ${CMAKE_CURRENT_SOURCE_DIR}/core/toolboxes.py "validate" ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/menudata.xml
   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/menudata.xml ${WXPYTHON_DIR}/xml/
   DEPENDS build_modules_items_xml)

  add_custom_target(build_module_tree_menudata
  COMMAND ${env_command} ${PYTHON_EXECUTABLE} 
   ${CMAKE_CURRENT_SOURCE_DIR}/core/toolboxes.py "module_tree" > ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/module_tree_menudata.xml
  COMMAND ${env_command} ${PYTHON_EXECUTABLE} 
   ${CMAKE_CURRENT_SOURCE_DIR}/core/toolboxes.py "validate" ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/module_tree_menudata.xml
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/module_tree_menudata.xml ${WXPYTHON_DIR}/xml/
  DEPENDS build_xml_menudata)

  add_custom_target(build_menustrings ALL 
  COMMAND ${env_command} ${PYTHON_EXECUTABLE} 
   ${CMAKE_CURRENT_SOURCE_DIR}/core/menutree.py "manager" >> ${CMAKE_CURRENT_SOURCE_DIR}/menustrings.py
  COMMAND ${env_command} ${PYTHON_EXECUTABLE} 
   ${CMAKE_CURRENT_SOURCE_DIR}/core/menutree.py "module_tree" >> ${CMAKE_CURRENT_SOURCE_DIR}/menustrings.py
  COMMAND ${env_command} ${PYTHON_EXECUTABLE} 
   ${CMAKE_CURRENT_SOURCE_DIR}/core/menutree.py "psmap" >> ${CMAKE_CURRENT_SOURCE_DIR}/menustrings.py
  DEPENDS build_module_tree_menudata)


endif()
