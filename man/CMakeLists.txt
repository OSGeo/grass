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

# TODO(FHS): remove this if condition
if(NOT WITH_FHS)

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
set_target_properties(build_full_index PROPERTIES FOLDER Man)

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
set_target_properties(build_index PROPERTIES FOLDER Man)

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
  set_target_properties(${target_name} PROPERTIES FOLDER Man)
endforeach()

add_custom_target(
  build_class_graphical
  COMMAND
    ${grass_env_command} ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/build_class_graphical.py
    html
    ${OUTDIR}/${GRASS_INSTALL_DOCDIR}
  COMMAND
    ${grass_env_command} ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/build_class_graphical.py
    md
    ${OUTDIR}/${GRASS_INSTALL_MKDOCSDIR}/source
  DEPENDS build_index
  COMMENT "man generation: build_class_graphical")
set_target_properties(build_class_graphical PROPERTIES FOLDER Man)

add_custom_target(
  build_pso
  COMMAND
    ${grass_env_command} ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/parser_standard_options.py -t
    "${CMAKE_SOURCE_DIR}/lib/gis/parser_standard_options.c" -f "grass" -o
    "${OUTDIR}/${GRASS_INSTALL_DOCDIR}/parser_standard_options.html" -p
    "id='opts_table' class='scroolTable'"
  COMMAND
    ${grass_env_command} ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/parser_standard_options.py -t
    "${CMAKE_SOURCE_DIR}/lib/gis/parser_standard_options.c" -f "grass" -o
    "${OUTDIR}/${GRASS_INSTALL_MKDOCSDIR}/source/parser_standard_options.md"
  DEPENDS ${target_names}
  COMMENT "man generation: parser standard options")
set_target_properties(build_pso PROPERTIES FOLDER Man)

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
      ${YEAR}
    DEPENDS build_pso
    COMMENT "man generation: build class ${class_name}")
  set_target_properties(build_class_${class_name} PROPERTIES FOLDER Man)
endforeach()

set(mkdocs_dir ${OUTDIR}/${GRASS_INSTALL_MKDOCSDIR})
add_custom_target(
  copy_mkdocs_files
  COMMAND
    ${CMAKE_COMMAND} -E make_directory ${mkdocs_dir}/scripts
    ${mkdocs_dir}/overrides/partials
    ${mkdocs_dir}/overrides/fragments/tags/default
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_CURRENT_SOURCE_DIR}/mkdocs/mkdocs.yml ${mkdocs_dir}
  COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/mkdocs/index.md
    ${CMAKE_CURRENT_SOURCE_DIR}/mkdocs/keywords.md
    ${CMAKE_CURRENT_SOURCE_DIR}/mkdocs/grassdocs.css
    ${CMAKE_CURRENT_SOURCE_DIR}/mkdocs/docs/development_intro.md
    ${CMAKE_CURRENT_SOURCE_DIR}/mkdocs/docs/command_line_intro.md
    ${CMAKE_CURRENT_SOURCE_DIR}/grass_logo.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/grass_icon.png
    ${CMAKE_CURRENT_SOURCE_DIR}/favicon.ico ${mkdocs_dir}/source
  COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/mkdocs/scripts/hook_list_scripts.py
    ${mkdocs_dir}/scripts
  COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/mkdocs/overrides/partials/copyright.html
    ${CMAKE_CURRENT_SOURCE_DIR}/mkdocs/overrides/partials/footer.html
    ${CMAKE_CURRENT_SOURCE_DIR}/mkdocs/overrides/partials/actions.html
    ${CMAKE_CURRENT_SOURCE_DIR}/mkdocs/overrides/partials/source.html
    ${mkdocs_dir}/overrides/partials
  COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/mkdocs/overrides/fragments/tags/default/listing.html
    ${mkdocs_dir}/overrides/fragments/tags/default
  COMMENT "man generation: copy mkdocs site files")
set_target_properties(copy_mkdocs_files PROPERTIES FOLDER Man)

# Pages in the markdown tree that exist only for the documentation website
# and have no man page equivalent (WEB_ONLY_MD in man/Makefile). The guides
# are derived from the doc/ source directories so a newly added guide is
# excluded automatically; grass_database and projectionintro are reference
# pages that also ship as man pages.
set(web_only_pages index keywords development_intro command_line_intro)
file(GLOB doc_md ${CMAKE_SOURCE_DIR}/doc/*.md
     ${CMAKE_SOURCE_DIR}/doc/development/*.md)
foreach(doc_file ${doc_md})
  cmake_path(GET doc_file STEM page_name)
  list(APPEND web_only_pages ${page_name})
endforeach()
list(REMOVE_ITEM web_only_pages grass_database projectionintro)

add_custom_target(
  build_manpages
  COMMAND
    ${grass_env_command} ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/build_manpages.py
    ${OUTDIR}/${GRASS_INSTALL_MKDOCSDIR}/source ${OUTDIR}/${GRASS_INSTALL_MANDIR}
    ${MD2MAN} --exclude ${web_only_pages}
  DEPENDS ${category_targets} build_class_graphical
  COMMENT "man generation: man pages from markdown")
set_target_properties(build_manpages PROPERTIES FOLDER Man)

# TODO: this shouldn't depend on GUI_WXPYTHON
add_custom_target(
  build_check ALL
  COMMAND
    ${grass_env_command} ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/build_check.py ${OUTDIR}/${GRASS_INSTALL_DOCDIR}
  DEPENDS ${category_targets} build_class_graphical build_manpages
          copy_mkdocs_files ALL_MODULES LIB_PYTHON GUI_WXPYTHON
  COMMENT "man generation: check output")
set_target_properties(build_check PROPERTIES FOLDER Man)

# The MkDocs site is built on demand (requires mkdocs installed), matching
# the Autotools build-mkdocs and serve-mkdocs targets.
# $<SEMICOLON> keeps the ; of &copy; from being parsed as a list separator.
set(mkdocs_env
    ${CMAKE_COMMAND} -E env
    "SITE_NAME=GRASS ${GRASS_VERSION_NUMBER} Documentation"
    "COPYRIGHT=&copy$<SEMICOLON> 2003-${GRASS_VERSION_DATE} GRASS Development Team, GRASS ${GRASS_VERSION_NUMBER} Documentation"
)
add_custom_target(
  build-mkdocs
  COMMAND ${mkdocs_env} mkdocs build
  WORKING_DIRECTORY ${mkdocs_dir}
  DEPENDS copy_mkdocs_files
  VERBATIM
  COMMENT "Building the MkDocs documentation site")
set_target_properties(build-mkdocs PROPERTIES FOLDER Man)

add_custom_target(
  serve-mkdocs
  COMMAND ${mkdocs_env} mkdocs serve
  WORKING_DIRECTORY ${mkdocs_dir}
  DEPENDS copy_mkdocs_files
  USES_TERMINAL
  VERBATIM
  COMMENT "Serving the MkDocs documentation site")
set_target_properties(serve-mkdocs PROPERTIES FOLDER Man)

endif() # NOT WITH_FHS
