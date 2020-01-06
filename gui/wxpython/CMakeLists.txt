
#missing docs
set(gui_sub_dirs animation core datacatalog dbmgr gcp gmodeler gui_core iclass
icons image2target iscatt lmgr location_wizard mapdisp mapswipe mapwin modules nviz
photo2image psmap rdigit rlisetup startup timeline tools tplot vdigit
vnet web_services wxplot)

foreach(gui_sub_dir ${gui_sub_dirs})
  build_pymodule_in_subdir(${gui_sub_dir} gui/wxpython/${gui_sub_dir})
endforeach()

install(FILES README DESTINATION gui/wxpython)

copy_python_file(${CMAKE_CURRENT_SOURCE_DIR}/gis_set.py gui/wxpython)
copy_python_file(${CMAKE_CURRENT_SOURCE_DIR}/gis_set_error.py gui/wxpython)
copy_python_file(${CMAKE_CURRENT_SOURCE_DIR}/wxgui.py gui/wxpython)

get_property(MODULE_LIST GLOBAL PROPERTY MODULE_LIST)
separate_arguments(MODULE_LIST)
set(EXCLUDED_MODULE_LIST "current_time_s_ms;g.parser;cell;vector;v.in.pdal;i.find")
list(REMOVE_ITEM MODULE_LIST ${EXCLUDED_MODULE_LIST})
list(REMOVE_DUPLICATES MODULE_LIST )

set(WXPYTHON_DIR ${GISBASE}/gui/wxpython)

add_custom_target(copy_build_module_xml 
  #COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_modules_xml.py ${WXPYTHON_DIR}/tools/
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../xml/grass-interface.dtd ${GISBASE}/gui/
  COMMAND ${PYTHON_EXECUTABLE} -m compileall ${GISBASE}/scripts
  COMMAND ${PYTHON_EXECUTABLE} -m compileall ${WXPYTHON_DIR}
  DEPENDS copy_gui_images
  )

  add_custom_target(build_modules_items_xml 
    COMMAND ${RUN_PYTHON}
  ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_modules_xml.py > ${WXPYTHON_DIR}/xml/module_items.xml
  DEPENDS copy_build_module_xml
  )

   add_custom_target(build_xml_menudata
   COMMAND ${RUN_PYTHON} 
   ${WXPYTHON_DIR}/core/toolboxes.py > ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/menudata.xml
   COMMAND ${RUN_PYTHON}
   ${WXPYTHON_DIR}/core/toolboxes.py "validate" ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/menudata.xml
   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/menudata.xml ${WXPYTHON_DIR}/xml
   DEPENDS build_modules_items_xml)

  add_custom_target(build_module_tree_menudata
  COMMAND ${RUN_PYTHON}
  ${WXPYTHON_DIR}/core/toolboxes.py "module_tree" > ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/module_tree_menudata.xml
  COMMAND ${RUN_PYTHON}
  ${WXPYTHON_DIR}/core/toolboxes.py "validate" ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/module_tree_menudata.xml
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/module_tree_menudata.xml ${WXPYTHON_DIR}/xml
  DEPENDS build_xml_menudata)

  add_custom_target(build_menustrings ALL 
  COMMAND ${RUN_PYTHON}
  ${WXPYTHON_DIR}/core/menutree.py "manager" >> ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/menustrings.py
  COMMAND ${RUN_PYTHON}
  ${WXPYTHON_DIR}/core/menutree.py "module_tree" >> ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/menustrings.py
  COMMAND ${RUN_PYTHON}
  ${WXPYTHON_DIR}/core/menutree.py "psmap" >> ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/menustrings.py
  DEPENDS build_module_tree_menudata)


  #install after validate ${CMAKE_CURRENT_BINARY_DIR}/wxpython/xml/module_tree_menudata.xml
  #python3 core/menutree.py "" >> menustrings.py
  #python3 core/menutree.py "module_tree" >> menustrings.py
  #python3 core/menutree.py "" >> menustrings.py
