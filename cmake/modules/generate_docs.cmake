#[[
AUTHOR(S):  Nicklas Larsson
PURPOSE:    Generate documentation (html, man etc.)
COPYRIGHT:  (C) 2025-2026 by the GRASS Development Team
SPDX-License-Identifier: GPL-2.0-or-later
]]

#[=======================================================================[.rst:

generate_docs
-------------

Generate documentation

  generate_docs_list([TARGET <target>]
                     [DOC_FILES <doc-files>...]
                     [IMG_FILES <image-files>...]
                     [MD_ONLY])

  Generate documentation files (.html, .md, man page) of <doc-files>, which
  is a list of file names without file extension. The image files
  <image-files> are installed. The command is added to the target <target>.
  This is a command intended for plain documentation files, not modules.
  With MD_ONLY, only the Markdown page is generated: for the web-only
  guides, which have no .html source and are not shipped as man pages.

  generate_docs(<name>
              [SOURCEDIR <source-directory>]
              [TARGET <target>]
              [OUTPUT <output-html-file>]
              [DEST_DIR <destination-directory>]
              [GUI_TARGET_NAME <gui-target-name>]
              [DEPENDS <dependency-targets>...]
              [IMG_FILES <image-files>...]
              [IMG_NO]
              [HTML_DESCR]
              [MD_ONLY])

  Generate documentation files (.html, .md, man page) of <name>, which is a
  source documentation file name without extension. The man page is built
  from the generated Markdown; the HTML pages are generated for the legacy
  documentation system. With MD_ONLY, only the Markdown page is generated
  (no .html, no man page); it applies only together with TARGET.

  Typical use case is, for example:

  `generate_docs(<name> TARGET <target>)`

  Which generates documentation based on file(s) named <name>[.html|.md],
  added as a POST_BUILD command to <target>.

#]=======================================================================]

macro(generate_docs_list)
  cmake_parse_arguments(D "MD_ONLY" "TARGET" "DOC_FILES;IMG_FILES" ${ARGN})

  if(NOT D_TARGET OR NOT D_DOC_FILES)
    message(FATAL_ERROR "TARGET >${D_TARGET}< or DOC_FILES >${D_DOC_FILES}< in not set")
  endif()

  if(D_MD_ONLY)
    set(md_only_flag MD_ONLY)
  else()
    set(md_only_flag)
  endif()

  foreach(doc_file ${D_DOC_FILES})
    generate_docs(${doc_file} TARGET ${D_TARGET} SOURCEDIR ${CMAKE_CURRENT_SOURCE_DIR}
                  IMG_NO ${md_only_flag})
  endforeach()

  if(D_IMG_FILES)
    add_custom_command(
      TARGET ${D_TARGET}
      PRE_BUILD
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${D_IMG_FILES} ${OUTDIR}/${GRASS_INSTALL_DOCDIR}
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${D_IMG_FILES}
              ${OUTDIR}/${GRASS_INSTALL_MKDOCSDIR}/source)
    install(FILES ${D_IMG_FILES} DESTINATION ${GRASS_INSTALL_DOCDIR})
    install(FILES ${D_IMG_FILES} DESTINATION ${GRASS_INSTALL_MKDOCSDIR}/source)
  endif()
endmacro()

function(generate_docs name)
  cmake_parse_arguments(PARSE_ARGV 1 D
    "IMG_NO;HTML_DESCR;MD_ONLY"
    "SOURCEDIR;TARGET;OUTPUT;DEST_DIR;GUI_TARGET_NAME"
    "IMG_FILES;DEPENDS")

  if(NOT D_TARGET AND NOT D_OUTPUT)
    message(FATAL_ERROR "TARGET or OUTPUT in not set")
  endif()

  set(sourcedir "${D_SOURCEDIR}")
  if(NOT D_SOURCEDIR)
    set(sourcedir "${CMAKE_CURRENT_SOURCE_DIR}")
  endif()

  if(D_GUI_TARGET_NAME)
    set(html_file_name ${D_GUI_TARGET_NAME}.html)
    set(tmp_html_name ${D_GUI_TARGET_NAME}.tmp.html)
    set(md_file_name ${D_GUI_TARGET_NAME}.md)
    set(tmp_md_name ${D_GUI_TARGET_NAME}.tmp.md)
    set(man_file_name ${D_GUI_TARGET_NAME}.1)
  else()
    set(html_file_name ${name}.html)
    set(tmp_html_name ${name}.tmp.html)
    set(md_file_name ${name}.md)
    set(tmp_md_name ${name}.tmp.md)
    set(man_file_name ${name}.1)
  endif()
  # The doc generation commands run in the source directory: mkhtml.py and
  # mkmarkdown.py read the page source and the .tmp. usage file from the
  # current directory and derive the repository path for the source code
  # links from it. The .tmp. files are gitignored, and the Autotools build
  # writes them into the source directory as well.
  set(tmp_html_file "${sourcedir}/${tmp_html_name}")
  set(out_html_file "${OUTDIR}/${GRASS_INSTALL_DOCDIR}/${html_file_name}")
  set(mkdocs_source_dir "${OUTDIR}/${GRASS_INSTALL_MKDOCSDIR}/source")
  set(tmp_md_file "${sourcedir}/${tmp_md_name}")
  set(out_md_file "${mkdocs_source_dir}/${md_file_name}")
  set(out_man_file "${OUTDIR}/${GRASS_INSTALL_MANDIR}/${man_file_name}")

  if(WIN32)
    set(ext ".exe")
    set(py ".py")
    set(null_device nul)
    set(search_cmd findstr /r /v)
    set(html_search_str "\"</body> </html> </div>.<!--.end.container.-->\"")
  else()
    set(ext "")
    set(py "")
    set(null_device /dev/null)
    set(search_cmd grep -v)
    set(html_search_str "'</body>\|</html>\|</div>.<!--.end.container.-->'")
  endif()

  if(D_HTML_DESCR)
    set(html_descr_command
        ${grass_env_command}
        ${name}${ext} --html-description < ${null_device} |
        ${search_cmd} ${html_search_str})
    set(md_descr_command ${grass_env_command} ${name}${ext} --md-description
        < ${null_device})
  else()
    set(html_descr_command ${CMAKE_COMMAND} -E echo)
    set(md_descr_command ${CMAKE_COMMAND} -E echo)
  endif()

  if(NOT D_IMG_FILES)
    file(GLOB D_IMG_FILES
         LIST_DIRECTORIES FALSE
         ${sourcedir}/*.png ${sourcedir}/*.jpg)
  endif()
  if(D_IMG_FILES AND NOT D_IMG_NO)
    set(copy_images_command ${CMAKE_COMMAND} -E copy_if_different ${D_IMG_FILES}
                            ${OUTDIR}/${GRASS_INSTALL_DOCDIR})
    set(copy_images_command_md ${CMAKE_COMMAND} -E copy_if_different
                               ${D_IMG_FILES} ${mkdocs_source_dir})
    install(FILES ${D_IMG_FILES} DESTINATION ${GRASS_INSTALL_DOCDIR})
    install(FILES ${D_IMG_FILES} DESTINATION ${GRASS_INSTALL_MKDOCSDIR}/source)
  endif()

  set(mkhtml_cmd ${grass_env_command} ${PYTHON_EXECUTABLE} ${MKHTML_PY})
  set(mkmarkdown_cmd ${grass_env_command} ${PYTHON_EXECUTABLE} ${MKMARKDOWN_PY})
  set(md2man_cmd ${grass_env_command} ${PYTHON_EXECUTABLE} ${MD2MAN})

  if(D_TARGET AND D_MD_ONLY)
    add_custom_command(
      TARGET ${D_TARGET}
      POST_BUILD
      WORKING_DIRECTORY ${sourcedir}
      COMMAND ${md_descr_command} > ${tmp_md_file}
      COMMAND ${mkmarkdown_cmd} ${name} > ${out_md_file}
      COMMAND ${copy_images_command_md}
      COMMAND ${CMAKE_COMMAND} -E remove ${tmp_md_file}
      BYPRODUCTS ${out_md_file}
      COMMENT "Creating ${name}.md")
  elseif(D_TARGET)
    add_custom_command(
      TARGET ${D_TARGET}
      POST_BUILD
      WORKING_DIRECTORY ${sourcedir}
      COMMAND ${html_descr_command} > ${tmp_html_file}
      COMMAND ${mkhtml_cmd} ${name} > ${out_html_file}
      COMMAND ${md_descr_command} > ${tmp_md_file}
      COMMAND ${mkmarkdown_cmd} ${name} > ${out_md_file}
      COMMAND ${md2man_cmd} ${out_md_file} ${out_man_file}
      COMMAND ${copy_images_command}
      COMMAND ${copy_images_command_md}
      COMMAND ${CMAKE_COMMAND} -E remove ${tmp_html_file} ${tmp_md_file}
      BYPRODUCTS ${out_md_file} ${out_man_file}
      COMMENT "Creating ${name}.[html|md|1]")
  elseif(D_GUI_TARGET_NAME)
    set(gui_html_file ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/wxGUI.${name}.html)
    set(gui_md_file ${mkdocs_source_dir}/wxGUI.${name}.md)
    set(gui_man_file ${OUTDIR}/${GRASS_INSTALL_MANDIR}/wxGUI.${name}.1)
    set(py_script_file ${OUTDIR}/${GRASS_INSTALL_SCRIPTDIR}/${D_GUI_TARGET_NAME}${py})

    add_custom_command(
      OUTPUT ${out_html_file}
      WORKING_DIRECTORY ${sourcedir}
      COMMAND ${grass_env_command} ${PYTHON_EXECUTABLE} ${py_script_file}
              --html-description < ${null_device} | ${search_cmd}
              ${html_search_str} > ${tmp_html_file}
      COMMAND ${mkhtml_cmd} ${D_GUI_TARGET_NAME} > ${out_html_file}
      COMMAND ${copy_images_command}
      COMMAND ${CMAKE_COMMAND} -E remove ${tmp_html_file}
      COMMAND ${mkhtml_cmd} ${D_GUI_TARGET_NAME} > ${gui_html_file}
      COMMAND ${grass_env_command} ${PYTHON_EXECUTABLE} ${py_script_file}
              --md-description < ${null_device} > ${tmp_md_file}
      COMMAND ${mkmarkdown_cmd} ${D_GUI_TARGET_NAME} > ${out_md_file}
      COMMAND ${md2man_cmd} ${out_md_file} ${out_man_file}
      COMMAND ${copy_images_command_md}
      COMMAND ${CMAKE_COMMAND} -E remove ${tmp_md_file}
      COMMAND ${mkmarkdown_cmd} ${D_GUI_TARGET_NAME} > ${gui_md_file}
      COMMAND ${md2man_cmd} ${gui_md_file} ${gui_man_file}
      BYPRODUCTS ${out_md_file} ${out_man_file} ${gui_md_file} ${gui_man_file}
      COMMENT "Creating ${out_html_file} and ${gui_html_file}"
      DEPENDS ${D_DEPENDS})
  else()
    set(out_html_file "${D_OUTPUT}")
    set(py_script_file "${OUTDIR}/${D_DEST_DIR}/${name}${py}")

    add_custom_command(
      OUTPUT ${out_html_file}
      WORKING_DIRECTORY ${sourcedir}
      COMMAND ${grass_env_command} ${PYTHON_EXECUTABLE} ${py_script_file}
              --html-description < ${null_device} | ${search_cmd}
              ${html_search_str} > ${tmp_html_file}
      COMMAND ${mkhtml_cmd} ${name} > ${out_html_file}
      COMMAND ${grass_env_command} ${PYTHON_EXECUTABLE} ${py_script_file}
              --md-description < ${null_device} > ${tmp_md_file}
      COMMAND ${mkmarkdown_cmd} ${name} > ${out_md_file}
      COMMAND ${md2man_cmd} ${out_md_file} ${out_man_file}
      COMMAND ${copy_images_command}
      COMMAND ${copy_images_command_md}
      COMMAND ${CMAKE_COMMAND} -E remove ${tmp_html_file} ${tmp_md_file}
      BYPRODUCTS ${out_md_file} ${out_man_file}
      COMMENT "Creating ${OUT_HTML_FILE}"
      DEPENDS ${D_DEPENDS})
  endif()

endfunction()
