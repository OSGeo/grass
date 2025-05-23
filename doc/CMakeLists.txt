file(GLOB doc_HTMLFILES "*.html")
add_custom_target(ADD_DOC_DOCS ALL COMMENT "Generate doc/html docs." DEPENDS LIB_PYTHON)

foreach(html_file ${doc_HTMLFILES})
  get_filename_component(filename ${html_file} NAME_WLE)
  generate_html(TARGET ADD_DOC_DOCS NAME ${filename} IMG_NOT)
endforeach()

file(
  GLOB IMG_FILES
  LIST_DIRECTORIES FALSE
  *.png *.jpg)
if(IMG_FILES)
  install(FILES ${IMG_FILES} DESTINATION ${GRASS_INSTALL_DOCDIR})
endif()
