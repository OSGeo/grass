execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory
                        etc/python/grass/pygrass/)
copy_python_file(${CMAKE_CURRENT_SOURCE_DIR}/errors.py etc/python/grass/pygrass)
copy_python_file(${CMAKE_CURRENT_SOURCE_DIR}/utils.py etc/python/grass/pygrass)
copy_python_file(${CMAKE_CURRENT_SOURCE_DIR}/orderdict.py
                 etc/python/grass/pygrass)

build_pymodule_in_subdir(messages etc/python/grass/pygrass)

build_pymodule_in_subdir(modules etc/python/grass/pygrass)

build_pymodule_in_subdir(modules/interface etc/python/grass/pygrass/modules)

build_pymodule_in_subdir(modules/grid etc/python/grass/pygrass/modules)

build_pymodule_in_subdir(raster etc/python/grass/pygrass)

build_pymodule_in_subdir(vector etc/python/grass/pygrass)

build_pymodule_in_subdir(gis etc/python/grass/pygrass)

build_pymodule_in_subdir(shell etc/python/grass/pygrass)

build_pymodule_in_subdir(tests etc/python/grass/pygrass)

build_pymodule_in_subdir(rpc etc/python/grass/pygrass)
