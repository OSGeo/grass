string(TIMESTAMP YEAR %Y)

add_custom_target(
  build_full_index
  COMMAND
    ${grass_env_command} ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/build_full_index.py
    ${YEAR}
  BYPRODUCTS ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/full_index.html
  DEPENDS ALL_MODULES LIB_PYTHON GUI_WXPYTHON
  COMMENT "man generation: build full index")

set(data_files
    ${CMAKE_CURRENT_SOURCE_DIR}/jquery.fixedheadertable.min.js
    ${CMAKE_CURRENT_SOURCE_DIR}/grassdocs.css
    ${CMAKE_CURRENT_SOURCE_DIR}/hamburger_menu.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/grass_icon.png
    ${CMAKE_CURRENT_SOURCE_DIR}/grass_logo.png
    ${CMAKE_CURRENT_SOURCE_DIR}/hamburger_menu_close.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/parser_standard_options.css
    ${CMAKE_CURRENT_SOURCE_DIR}/parser_standard_options.js)
install(FILES ${data_files} DESTINATION ${GRASS_INSTALL_DOCDIR})

add_custom_command(
  TARGET build_full_index
  PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${data_files}
          ${OUTDIR}/${GRASS_INSTALL_DOCDIR}
  COMMENT "copying man files")

add_custom_target(
  build_index
  COMMAND
    ${grass_env_command} ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/build_index.py ${OUTDIR}/${GRASS_INSTALL_DOCDIR}
  BYPRODUCTS ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/index.html
  DEPENDS build_full_index
  COMMENT "man generation: build index")

set(target_names build_topics build_keywords build_graphical_index
                 build_manual_gallery)
foreach(target_name ${target_names})
  add_custom_target(
    ${target_name}
    COMMAND
      ${grass_env_command} ${PYTHON_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/${target_name}.py
    DEPENDS build_index
    COMMENT "man generation: ${target_name}")
endforeach()

add_custom_target(
  build_class_graphical
  COMMAND
    ${grass_env_command} ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/build_class_graphical.py
    html
    ${OUTDIR}/${GRASS_INSTALL_DOCDIR}
  DEPENDS build_index
  COMMENT "man generation: build_class_graphical")

add_custom_target(
  build_pso
  COMMAND
    ${grass_env_command} ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/parser_standard_options.py -t
    "${CMAKE_SOURCE_DIR}/lib/gis/parser_standard_options.c" -f "grass" -o
    "${OUTDIR}/${GRASS_INSTALL_DOCDIR}/parser_standard_options.html" -p
    "id='opts_table' class='scroolTable'"
  DEPENDS ${target_names}
  COMMENT "man generation: parser standard options")

set(categories
    d:display
    db:database
    g:general
    i:imagery
    m:miscellaneous
    ps:postscript
    r:raster
    r3:raster3d
    t:temporal
    v:vector)
set(category_targets "")
foreach(category ${categories})
  string(REPLACE ":" ";" category_list ${category})
  list(GET category_list 0 prefix)
  list(GET category_list 1 class_name)
  list(APPEND category_targets build_class_${class_name})
  add_custom_target(
    build_class_${class_name}
    COMMAND
      ${grass_env_command} ${PYTHON_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/build_class.py ${prefix} ${class_name}
      ${OUTDIR}/${GRASS_INSTALL_DOCDIR}
    DEPENDS build_pso
    COMMENT "man generation: build class ${class_name}")
endforeach()

# TODO: this shouldn't depend on GUI_WXPYTHON
add_custom_target(
  build_check ALL
  COMMAND
    ${grass_env_command} ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/build_check.py ${OUTDIR}/${GRASS_INSTALL_DOCDIR}
  DEPENDS ${category_targets} ALL_MODULES LIB_PYTHON GUI_WXPYTHON
  COMMENT "man generation: check output")

#[=======[ TODO: implement somehow...
add_custom_target(
  create_man_pages ALL
  DEPENDS build_check
  COMMAND ${CMAKE_COMMAND} -DHTML2MAN=${HTML2MAN}
    -DGRASS_INSTALL_DOCDIR=${GRASS_INSTALL_DOCDIR} -DOUTDIR=${OUTDIR}
    -DGRASS_INSTALL_MANDIR=${GRASS_INSTALL_MANDIR} -P
    ${CMAKE_SOURCE_DIR}/cmake/generate_man_pages.cmake
  COMMENT "!!! man generation: create man pages")
]=======]
