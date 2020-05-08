
set(wxpython_html_files)

ADD_CUSTOM_COMMAND(OUTPUT ${GISBASE}/docs/html/wxGUI.components.html
  COMMAND ${grass_env_command}
  ${PYTHON_EXECUTABLE} ${MKHTML_PY} wxGUI.components ${GRASS_VERSION_DATE} > ${GISBASE}/docs/html/wxGUI.components.html
  COMMENT "Creating ${GISBASE}/docs/html/wxGUI.components.html"
  DEPENDS GUI_WXPYTHON LIB_PYTHON
  )

list(APPEND wxpython_html_files ${GISBASE}/docs/html/wxGUI.components.html)

ADD_CUSTOM_TARGET(wxpython_docs DEPENDS ${wxpython_html_files})
