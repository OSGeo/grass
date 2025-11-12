set(wxpython_html_files
    wxGUI.components
    wxGUI
    wxGUI.iscatt
    wxGUI.modules
    wxGUI.nviz
    wxGUI.toolboxes
    wxGUI.vnet)

add_custom_target(wxpython_docs DEPENDS grass_gis)
add_dependencies(GUI_WXPYTHON wxpython_docs)

foreach(html_file ${wxpython_html_files})
  add_custom_command(
    TARGET wxpython_docs
    PRE_BUILD DEPENDS LIB_PYTHON
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${grass_env_command} ${PYTHON_EXECUTABLE} ${MKHTML_PY} ${html_file}
            > ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/${html_file}.html
    BYPRODUCTS ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/${html_file}.html)
  install(FILES ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/${html_file}.html
          DESTINATION ${GRASS_INSTALL_DOCDIR})
endforeach()

file(
  GLOB img_files
  LIST_DIRECTORIES FALSE
  *.png *.jpg)

if(img_files)
  install(FILES ${img_files} DESTINATION ${GRASS_INSTALL_DOCDIR})
endif()
