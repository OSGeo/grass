#[===========================================================================[
  Currently this only installs mo files.
  TODO: implement update and creation
#]===========================================================================]

set(po_file_names
  grasslibs_ar.po
  grasslibs_bn.po
  grasslibs_cs.po
  grasslibs_de.po
  grasslibs_el.po
  grasslibs_es.po
  grasslibs_fi.po
  grasslibs_fr.po
  grasslibs_hu.po
  grasslibs_id_ID.po
  grasslibs_it.po
  grasslibs_ja.po
  grasslibs_ko.po
  grasslibs_lv.po
  grasslibs_ml.po
  grasslibs_pl.po
  grasslibs_pt_BR.po
  grasslibs_pt.po
  grasslibs_ro.po
  grasslibs_ru.po
  grasslibs_si.po
  grasslibs_sl.po
  grasslibs_sv.po
  grasslibs_ta.po
  grasslibs_th.po
  grasslibs_tr.po
  grasslibs_uk.po
  grasslibs_vi.po
  grasslibs_zh_CN.po
  grasslibs_zh.po
  grassmods_ar.po
  grassmods_bn.po
  grassmods_cs.po
  grassmods_de.po
  grassmods_el.po
  grassmods_es.po
  grassmods_fi.po
  grassmods_fr.po
  grassmods_hu.po
  grassmods_id_ID.po
  grassmods_it.po
  grassmods_ja.po
  grassmods_ko.po
  grassmods_lv.po
  grassmods_ml.po
  grassmods_pl.po
  grassmods_pt_BR.po
  grassmods_pt.po
  grassmods_ro.po
  grassmods_ru.po
  grassmods_si.po
  grassmods_sl.po
  grassmods_sv.po
  grassmods_ta.po
  grassmods_th.po
  grassmods_tr.po
  grassmods_uk.po
  grassmods_vi.po
  grassmods_zh_CN.po
  grassmods_zh.po
  grasswxpy_ar.po
  grasswxpy_bn.po
  grasswxpy_cs.po
  grasswxpy_de.po
  grasswxpy_el.po
  grasswxpy_es.po
  grasswxpy_fi.po
  grasswxpy_fr.po
  grasswxpy_hu.po
  grasswxpy_id_ID.po
  grasswxpy_it.po
  grasswxpy_ja.po
  grasswxpy_ko.po
  grasswxpy_lv.po
  grasswxpy_ml.po
  grasswxpy_pl.po
  grasswxpy_pt_BR.po
  grasswxpy_pt.po
  grasswxpy_ro.po
  grasswxpy_ru.po
  grasswxpy_si.po
  grasswxpy_sl.po
  grasswxpy_sv.po
  grasswxpy_ta.po
  grasswxpy_th.po
  grasswxpy_tr.po
  grasswxpy_uk.po
  grasswxpy_vi.po
  grasswxpy_zh_CN.po
  grasswxpy_zh.po
)

set(po_files ${po_file_names})
list(TRANSFORM po_files PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/po/")
set(generate_mo_file_targets ${po_file_names})
list(TRANSFORM generate_mo_file_targets PREPEND "generate_mo_files_")

add_custom_target(generate_mo_files_dir ALL COMMENT "Create locale directory")
set_target_properties(generate_mo_files_dir PROPERTIES FOLDER Locale)
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
  add_custom_target(${_locale}_LC_MESSAGES_dir ALL
                    COMMENT "Create locale (${_locale}) LC_MESSAGES directory")
  add_custom_command(
    TARGET ${_locale}_LC_MESSAGES_dir
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            ${OUTDIR}/${GRASS_INSTALL_LOCALEDIR}/${_locale}/LC_MESSAGES
    BYPRODUCTS ${OUTDIR}/${GRASS_INSTALL_LOCALEDIR}/${_locale}/LC_MESSAGES)

  set_target_properties(${_locale}_LC_MESSAGES_dir PROPERTIES FOLDER Locale)
  set_target_properties(generate_mo_files_dir_${_locale} PROPERTIES FOLDER
                                                                    Locale)
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
    DEPENDS ${_locale}_LC_MESSAGES_dir
    COMMENT "Generate mo files")
  set_target_properties(generate_mo_files_${po_file_name} PROPERTIES FOLDER
                                                                     Locale)
  add_custom_command(
    TARGET generate_mo_files_${po_file_name}
    POST_BUILD
    COMMAND ${MSGFMT} --statistics -o
            ${OUTDIR}/${GRASS_INSTALL_LOCALEDIR}/${mo_file} ${po_file}
    BYPRODUCTS ${OUTDIR}/${GRASS_INSTALL_LOCALEDIR}/${mo_file})
endforeach()

add_custom_target(translation_statistics ALL
  DEPENDS ${generate_mo_file_targets}
  COMMENT "Create translation statistics")
add_custom_command(
  TARGET translation_statistics
  PRE_BUILD
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND ${grass_env_command} ${PYTHON_EXECUTABLE}
          ${CMAKE_CURRENT_SOURCE_DIR}/grass_po_stats.py
  BYPRODUCTS ${OUTDIR}/${GRASS_INSTALL_MISCDIR}/translation_status.json)

set_target_properties(translation_statistics PROPERTIES FOLDER Locale)

install(DIRECTORY ${OUTDIR}/${GRASS_INSTALL_LOCALEDIR}/
        DESTINATION ${GRASS_INSTALL_LOCALEDIR})

# TODO(FHS): remove this if condition
if(NOT WITH_FHS)

install(FILES ${OUTDIR}/${GRASS_INSTALL_MISCDIR}/translation_status.json
        DESTINATION ${GRASS_INSTALL_MISCDIR})

endif() # NOT WITH_FHS
