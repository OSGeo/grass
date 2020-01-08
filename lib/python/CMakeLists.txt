
#include(build_py_module_in_subdir)
build_py_lib_in_subdir(exceptions)

build_py_lib_in_subdir(script)

configure_file(script/setup.py
  ${CMAKE_BINARY_DIR}/etc/python/grass/script/setup.py
  COPYONLY)

#  add_subdirectory(ctypes)
 
# build_py_lib_in_subdir(temporal etc/python/grass)

#  add_subdirectory(pygrass)

# build_py_lib_in_subdir(pydispatch etc/python/grass)
 
# build_py_lib_in_subdir(imaging etc/python/grass)
 
# build_py_lib_in_subdir(gunittest etc/python/grass)

 
file(WRITE ${GISBASE}/etc/python/_init__.py  "#coding=utf-8") 
file(COPY __init__.py DESTINATION ${GISBASE}/etc/python/grass/)
install(FILES __init__.py DESTINATION etc/python/grass/)
