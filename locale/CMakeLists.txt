#[===========================================================================[
  Currently this only installs mo files.
  TODO: implement update and creation
#]===========================================================================]

file(GLOB po_files "${CMAKE_CURRENT_SOURCE_DIR}/po/*.po")

add_custom_target(generate_mo_files_dir ALL COMMENT "Create locale directory")
add_custom_command(
  TARGET generate_mo_files_dir
  PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory
          ${OUTDIR}/${GRASS_INSTALL_LOCALEDIR}
  BYPRODUCTS ${OUTDIR}/${GRASS_INSTALL_LOCALEDIR})

set(_locales)
foreach(_locale ${po_files})
  get_filename_component(_loc ${_locale} NAME)
  list(APPEND _locales ${_loc})
endforeach()
list(TRANSFORM _locales REPLACE "^[^_]+_(.*)\.po$" "\\1")
list(REMOVE_DUPLICATES _locales)
foreach(_locale ${_locales})
  add_custom_target(
    generate_mo_files_dir_${_locale}
    DEPENDS generate_mo_files_dir
    COMMENT "Create locale specific directories")
  add_custom_command(
    OUTPUT ${OUTDIR}/${GRASS_INSTALL_LOCALEDIR}/${_locale}/LC_MESSAGES
    COMMAND ${CMAKE_COMMAND} -E make_directory
            ${OUTDIR}/${GRASS_INSTALL_LOCALEDIR}/${_locale}/LC_MESSAGES)
endforeach()
unset(_locales)

foreach(po_file ${po_files})
  get_filename_component(po_file_name ${po_file} NAME)
  string(REGEX REPLACE "^([^_]+)_(.*)\.po$" "\\2" _locale ${po_file_name})
  string(REGEX REPLACE "^([^_]+)_(.*)\.po$" "\\2/LC_MESSAGES/\\1.mo" mo_file
                       ${po_file_name})
  get_filename_component(mo_dir ${mo_file} DIRECTORY)

  add_custom_target(
    generate_mo_files_${po_file_name} ALL
    DEPENDS ${OUTDIR}/${GRASS_INSTALL_LOCALEDIR}/${_locale}/LC_MESSAGES
    COMMENT "Generate mo files")

  add_custom_command(
    TARGET generate_mo_files_${po_file_name}
    COMMAND ${MSGFMT} --statistics -o
            ${OUTDIR}/${GRASS_INSTALL_LOCALEDIR}/${mo_file} ${po_file}
    BYPRODUCTS ${OUTDIR}/${GRASS_INSTALL_LOCALEDIR}/${mo_file})
endforeach()

install(DIRECTORY ${OUTDIR}/${GRASS_INSTALL_LOCALEDIR}/
        DESTINATION ${GRASS_INSTALL_LOCALEDIR})
