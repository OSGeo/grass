#[===========================================================================[
  Currently this only installs mo files.
  TODO: implement update and creation
#]===========================================================================]

add_custom_target(generate_mo_files ALL COMMENT "Generate mo files")

add_custom_command(
  TARGET generate_mo_files
  PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory
          ${OUTDIR}/${GRASS_INSTALL_LOCALEDIR}
  BYPRODUCTS ${OUTDIR}/${GRASS_INSTALL_LOCALEDIR})

file(GLOB po_files "${CMAKE_CURRENT_SOURCE_DIR}/po/*.po")
foreach(po_file ${po_files})
  get_filename_component(po_file_name ${po_file} NAME)
  string(REGEX REPLACE "^([^_]+)_(.*)\.po" "\\2/LC_MESSAGES/\\1.mo" mo_file
                       ${po_file_name})
  get_filename_component(mo_dir ${mo_file} DIRECTORY)

  add_custom_command(
    TARGET generate_mo_files
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            ${OUTDIR}/${GRASS_INSTALL_LOCALEDIR}/${mo_dir}
    COMMAND ${MSGFMT} --statistics -o
            ${OUTDIR}/${GRASS_INSTALL_LOCALEDIR}/${mo_file} ${po_file}
    BYPRODUCTS ${OUTDIR}/${GRASS_INSTALL_LOCALEDIR}/${mo_file}
               ${OUTDIR}/${GRASS_INSTALL_LOCALEDIR}/${mo_dir})
endforeach()

install(DIRECTORY ${OUTDIR}/${GRASS_INSTALL_LOCALEDIR}/
        DESTINATION ${GRASS_INSTALL_LOCALEDIR})
