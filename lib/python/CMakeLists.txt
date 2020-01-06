
include(build_pymodule_in_subdir)
build_pymodule_in_subdir(exceptions etc/python/grass)

build_pymodule_in_subdir(script etc/python/grass)

configure_file(script/setup.py
  ${CMAKE_BINARY_DIR}/etc/python/grass/script/setup.py
  COPYONLY)

 add_subdirectory(ctypes)
 
 build_pymodule_in_subdir(temporal etc/python/grass)

 add_subdirectory(pygrass)

 build_pymodule_in_subdir(pydispatch etc/python/grass)
 
 build_pymodule_in_subdir(imaging etc/python/grass)
 
 build_pymodule_in_subdir(gunittest etc/python/grass)

 
file(WRITE ${GISBASE}/etc/python/_init__.py  "#coding=utf-8") 
file(COPY __init__.py DESTINATION ${GISBASE}/etc/python/grass/)
install(FILES __init__.py DESTINATION etc/python/grass/)
