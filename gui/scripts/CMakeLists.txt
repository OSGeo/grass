#[[
TODO: maybe add /scripts/windows_launch.bat, see Makefile
]]

set(gui_scripts d.rast3d d.wms)

foreach(script_file ${gui_scripts})
  build_script_in_subdir(${script_file} PLAIN_PY NO_DOCS DEST_DIR
                         ${GRASS_INSTALL_GUISCRIPTDIR})
endforeach()
