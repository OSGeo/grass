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
                     [IMG_FILES <image-files>...])

  Generate documentation files (.html, .man1 etc.) of <doc-files>, which is
  a list of file names without file extension. The image files <image-files>
  are installed. The command is added to the target <target>.
  This is a command intended for plain documentation files, not modules.

  generate_docs(<name>
              [SOURCEDIR <source-directory>]
              [TARGET <target>]
              [OUTPUT <output-html-file>]
              [DEST_DIR <destination-directory>]
              [GUI_TARGET_NAME <gui-target-name>]
              [DEPENDS <dependency-targets>...]
              [IMG_FILES <image-files>...]
              [IMG_NO]
              [HTML_DESCR])

  Generate documentation files (.html, .man1 etc.) of <name>, which is a source
  documentation file name without extension.

  Typical use case is, for example:

  `generate_docs(<name> TARGET <target>)`

  Which generates documentation based on file(s) named <name>[.html|.md],
  added as a POST_BUILD command to <target>.

#]=======================================================================]

macro(generate_docs_list)
  cmake_parse_arguments(D "" "TARGET" "DOC_FILES;IMG_FILES" ${ARGN})

  if(NOT D_TARGET OR NOT D_DOC_FILES)
    message(FATAL_ERROR "TARGET >${D_TARGET}< or DOC_FILES >${D_DOC_FILES}< in not set")
  endif()

  foreach(doc_file ${D_DOC_FILES})
    generate_docs(${doc_file} TARGET ${D_TARGET} SOURCEDIR ${CMAKE_CURRENT_SOURCE_DIR} IMG_NO)
  endforeach()

  if(D_IMG_FILES)
    add_custom_command(
      TARGET ${D_TARGET}
      PRE_BUILD
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${D_IMG_FILES} ${OUTDIR}/${GRASS_INSTALL_DOCDIR})
    install(FILES ${D_IMG_FILES} DESTINATION ${GRASS_INSTALL_DOCDIR})
  endif()
endmacro()

function(generate_docs name)
  cmake_parse_arguments(PARSE_ARGV 1 D
    "IMG_NO;HTML_DESCR"
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
    set(man_file_name ${D_GUI_TARGET_NAME}.1)
  else()
    set(html_file_name ${name}.html)
    set(tmp_html_name ${name}.tmp.html)
    set(man_file_name ${name}.1)
  endif()
  set(source_html "${sourcedir}/${html_file_name}")
  set(html_file "${CMAKE_CURRENT_BINARY_DIR}/${html_file_name}")
  set(tmp_html_file "${CMAKE_CURRENT_BINARY_DIR}/${tmp_html_name}")
  set(out_html_file "${OUTDIR}/${GRASS_INSTALL_DOCDIR}/${html_file_name}")
  set(out_man_file "${OUTDIR}/${GRASS_INSTALL_MANDIR}/${man_file_name}")

  if(WIN32)
    set(ext ".exe")
    set(py ".py")
    set(null_device nul)
    set(search_cmd findstr /v)
    set(html_search_str "\"</body>\|</html>\|</div> <!-- end container -->\"")
  else()
    set(ext "")
    set(py "")
    set(null_device /dev/null)
    set(search_cmd grep -v)
    set(html_search_str "\'</body>\|</html>\|</div> <!-- end container -->\'")
  endif()

  if(D_HTML_DESCR)
    set(html_descr_command
        ${grass_env_command}
        ${name}${ext} --html-description < ${null_device} |
        ${search_cmd} ${html_search_str})
  else()
    set(html_descr_command ${CMAKE_COMMAND} -E echo)
  endif()

  if(NOT D_IMG_FILES)
    file(GLOB D_IMG_FILES
         LIST_DIRECTORIES FALSE
         ${sourcedir}/*.png ${sourcedir}/*.jpg)
  endif()
  if(D_IMG_FILES AND NOT D_IMG_NO)
    set(copy_images_command ${CMAKE_COMMAND} -E copy_if_different ${D_IMG_FILES}
                            ${OUTDIR}/${GRASS_INSTALL_DOCDIR})
    install(FILES ${D_IMG_FILES} DESTINATION ${GRASS_INSTALL_DOCDIR})
  endif()

  set(mkhtml_cmd ${grass_env_command} ${PYTHON_EXECUTABLE} ${MKHTML_PY})
  set(html2man_cmd ${grass_env_command} ${PYTHON_EXECUTABLE} ${HTML2MAN})

  if(D_TARGET)
    add_custom_command(
      TARGET ${D_TARGET}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${source_html} ${html_file}
      COMMAND ${html_descr_command} > ${tmp_html_file}
      COMMAND ${mkhtml_cmd} ${name} > ${out_html_file}
      COMMAND ${html2man_cmd} ${out_html_file} ${out_man_file}
      COMMAND ${copy_images_command}
      COMMAND ${CMAKE_COMMAND} -E remove ${html_file}
      COMMENT "Creating ${name}.[html|1]")
  elseif(D_GUI_TARGET_NAME)
    set(gui_html_file ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/wxGUI.${name}.html)
    set(gui_man_file ${OUTDIR}/${GRASS_INSTALL_MANDIR}/wxGUI.${name}.1)
    set(py_script_file ${OUTDIR}/${GRASS_INSTALL_SCRIPTDIR}/${D_GUI_TARGET_NAME}${py})

    add_custom_command(
      OUTPUT ${out_html_file}
      COMMAND ${CMAKE_COMMAND} -E copy ${source_html} ${html_file}
      COMMAND ${grass_env_command} ${PYTHON_EXECUTABLE} ${py_script_file}
              --html-description < ${null_device} | ${search_cmd}
              ${html_search_str} > ${tmp_html_file}
      COMMAND ${mkhtml_cmd} ${D_GUI_TARGET_NAME} > ${out_html_file}
      COMMAND ${html2man_cmd} ${out_html_file} ${out_man_file}
      COMMAND ${copy_images_command}
      COMMAND ${CMAKE_COMMAND} -E remove ${tmp_html_file}
      COMMAND ${mkhtml_cmd} ${D_GUI_TARGET_NAME} > ${gui_html_file}
      COMMAND ${html2man_cmd} ${gui_html_file} ${gui_man_file}
      COMMAND ${CMAKE_COMMAND} -E remove ${html_file}
      COMMENT "Creating ${out_html_file} and ${gui_html_file}"
      DEPENDS ${D_DEPENDS})
  else()
    set(out_html_file "${D_OUTPUT}")
    set(py_script_file "${OUTDIR}/${D_DEST_DIR}/${name}${py}")

    add_custom_command(
      OUTPUT ${out_html_file}
      COMMAND ${CMAKE_COMMAND} -E copy ${source_html} ${html_file}
      COMMAND ${grass_env_command} ${PYTHON_EXECUTABLE} ${py_script_file}
              --html-description < ${null_device} | ${search_cmd}
              ${html_search_str} > ${tmp_html_file}
      COMMAND ${mkhtml_cmd} ${name} > ${out_html_file}
      COMMAND ${html2man_cmd} ${out_html_file} ${out_man_file}
      COMMAND ${copy_images_command}
      COMMAND ${CMAKE_COMMAND} -E remove ${tmp_html_file} ${html_file}
      COMMENT "Creating ${OUT_HTML_FILE}"
      DEPENDS ${D_DEPENDS})
  endif()

endfunction()
