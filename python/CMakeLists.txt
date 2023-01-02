
copy_python_files_in_subdir(exceptions etc/python/grass)
copy_python_files_in_subdir(gunittest etc/python/grass)
copy_python_files_in_subdir(imaging etc/python/grass)
copy_python_files_in_subdir(pydispatch etc/python/grass)
copy_python_files_in_subdir(script etc/python/grass)
copy_python_files_in_subdir(semantic_label etc/python/grass)
copy_python_files_in_subdir(temporal etc/python/grass)
copy_python_files_in_subdir(pygrass etc/python/grass)
copy_python_files_in_subdir(pygrass/messages etc/python/grass)
copy_python_files_in_subdir(pygrass/modules etc/python/grass)
copy_python_files_in_subdir(pygrass/raster etc/python/grass)
copy_python_files_in_subdir(pygrass/vector etc/python/grass)
copy_python_files_in_subdir(pygrass/gis etc/python/grass)
copy_python_files_in_subdir(pygrass/shell etc/python/grass)
copy_python_files_in_subdir(pygrass/tests etc/python/grass)
copy_python_files_in_subdir(pygrass/rpc etc/python/grass)
copy_python_files_in_subdir(pygrass/modules/interface etc/python/grass)
copy_python_files_in_subdir(pygrass/modules/grid etc/python/grass)


configure_file(grass/script/setup.py
  ${CMAKE_BINARY_DIR}/etc/python/grass/script/setup.py
  COPYONLY)

add_custom_target(LIB_PYTHON
	COMMAND ${CMAKE_COMMAND} -E echo "#coding=utf-8" > ${GISBASE}/etc/python/__init__.py
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/__init__.py  ${GISBASE}/etc/python/grass/
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/etc/python/grass/script/setup.py ${GISBASE}/etc/python/grass/script/
	DEPENDS python_exceptions python_imaging python_pydispatch python_script python_semantic_label python_temporal
	python_pygrass python_pygrass_messages python_pygrass_modules python_pygrass_modules 
	python_pygrass_raster python_pygrass_vector python_pygrass_gis python_pygrass_shell 
	python_pygrass_tests python_pygrass_rpc
	python_pygrass_modules_interface
	python_pygrass_modules_grid)

#TODO: MODULES DISABLED
#  add_subdirectory(ctypes)

set_target_properties (LIB_PYTHON PROPERTIES FOLDER lib)

install(DIRECTORY ${GISBASE}/etc/python/grass DESTINATION etc/python/grass/)
