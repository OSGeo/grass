#TODO
add_subdirectory(icons)
add_subdirectory(images)
#add_subdirectory(scripts)

add_subdirectory(wxpython)

install(FILES xml/grass-interface.dtd  DESTINATION gui/xml)
get_property(MODULE_LIST GLOBAL PROPERTY MODULE_LIST)
separate_arguments(MODULE_LIST)
set(EXCLUDED_MODULE_LIST "current_time_s_ms;g.parser;cell;vector;v.in.pdal;i.find")
list(REMOVE_ITEM MODULE_LIST ${EXCLUDED_MODULE_LIST})
list(REMOVE_DUPLICATES MODULE_LIST )

  add_custom_target(copy_build_module_xml 
  COMMAND ${CMAKE_COMMAND} -E copy 
  ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_modules_xml.py ${CMAKE_CURRENT_BINARY_DIR}/tools/build_modules_xml.py
  DEPENDS copy_docs_html_icons copy_gui_symbols)

  add_custom_target(build_modulesitems_xml 
  COMMAND ${RUN_PYTHON} 
  ${CMAKE_CURRENT_BINARY_DIR}/tools/build_modules_xml.py ${MODULE_LIST} > ${CMAKE_BINARY_DIR}/gui/wxpython/xml/module_items.xml
  DEPENDS copy_build_module_xml)

   add_custom_target(build_xml_menudata
   COMMAND ${RUN_PYTHON} 
   ${CMAKE_CURRENT_BINARY_DIR}/wxpython/core/toolboxes.py > ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/menudata.xml
   COMMAND ${RUN_PYTHON}
   ${CMAKE_CURRENT_BINARY_DIR}/wxpython/core/toolboxes.py "validate" ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/menudata.xml
   COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/wxpython/xml
   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/menudata.xml ${CMAKE_CURRENT_BINARY_DIR}/wxpython/xml
   DEPENDS build_modulesitems_xml)

  add_custom_target(build_module_tree_menudata
  COMMAND ${RUN_PYTHON}
  ${CMAKE_CURRENT_BINARY_DIR}/wxpython/core/toolboxes.py "module_tree" > ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/module_tree_menudata.xml
  COMMAND ${RUN_PYTHON}
  ${CMAKE_CURRENT_BINARY_DIR}/wxpython/core/toolboxes.py "validate" ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/module_tree_menudata.xml
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/module_tree_menudata.xml ${CMAKE_CURRENT_BINARY_DIR}/wxpython/xml
  DEPENDS build_xml_menudata)

  add_custom_target(build_menustrings ALL 
  COMMAND ${RUN_PYTHON}
  ${CMAKE_CURRENT_BINARY_DIR}/wxpython/core/menutree.py "manager" >> ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/menustrings.py
  COMMAND ${RUN_PYTHON}
  ${CMAKE_CURRENT_BINARY_DIR}/wxpython/core/menutree.py "module_tree" >> ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/menustrings.py
  COMMAND ${RUN_PYTHON}
  ${CMAKE_CURRENT_BINARY_DIR}/wxpython/core/menutree.py "psmap" >> ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/menustrings.py
  DEPENDS build_module_tree_menudata)


  #install after validate ${CMAKE_CURRENT_BINARY_DIR}/wxpython/xml/module_tree_menudata.xml
  #python3 core/menutree.py "" >> menustrings.py
  #python3 core/menutree.py "module_tree" >> menustrings.py
  #python3 core/menutree.py "" >> menustrings.py
