
#missing docs
set(gui_sub_dirs animation core datacatalog dbmgr gcp gmodeler gui_core iclass
icons image2target iscatt lmgr location_wizard mapdisp mapswipe mapwin modules nviz
photo2image psmap rdigit rlisetup startup timeline tools tplot vdigit
vnet web_services wxplot)

foreach(gui_sub_dir ${gui_sub_dirs})
  build_pymodule_in_subdir(${gui_sub_dir} gui/wxpython)
endforeach()
copy_python_file(${CMAKE_CURRENT_SOURCE_DIR}/gis_set.py gui/wxpython)
copy_python_file(${CMAKE_CURRENT_SOURCE_DIR}/gis_set_error.py gui/wxpython)
copy_python_file(${CMAKE_CURRENT_SOURCE_DIR}/wxgui.py gui/wxpython)

install(FILES README DESTINATION gui/wxpython)
