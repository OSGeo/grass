set(PYDIRS
    benchmark
    exceptions
    grassdb
    gunittest
    imaging
    jupyter
    pydispatch
    pygrass
    pygrass/gis
    pygrass/messages
    pygrass/modules
    pygrass/modules/grid
    pygrass/modules/interface
    pygrass/raster
    pygrass/rpc
    pygrass/shell
    pygrass/tests
    pygrass/vector
    script
    semantic_label
    temporal
    utils)

set(PYDIR_GRASS ${GRASS_INSTALL_PYDIR}/grass)

foreach(pydir ${PYDIRS})
  copy_python_files_in_subdir(${pydir} ${PYDIR_GRASS})
endforeach()
copy_python_files_in_subdir(app ${PYDIR_GRASS} EXCLUDE resource_paths.py)

configure_file(__init__.py ${OUTDIR}/${PYDIR_GRASS}/ COPYONLY)
configure_file(script/setup.py ${OUTDIR}/${PYDIR_GRASS}/script/setup.py
               COPYONLY)


file(TO_NATIVE_PATH ${CMAKE_BINARY_DIR}/bin BINARY_DIR)
set(CONFIG_PROJSHARE)
get_filename_component(PROJ_INSTALL_PREFIX ${PROJ_INCLUDE_DIRS} PATH)
if(DEFINED ENV{PROJSHARE})
  message(WARNING "External PROJ directory not specified; default will be used")
  set(CONFIG_PROJSHARE "$ENV{PROJSHARE}")
else()
  set(CONFIG_PROJSHARE "${PROJ_INSTALL_PREFIX}/share/proj")
  if(EXISTS ${CONFIG_PROJSHARE}/epsg)
    message(STATUS "PROJ directory ${CONFIG_PROJSHARE}")
  endif()
endif()
file(TO_NATIVE_PATH "${PROJ_INSTALL_PREFIX}/share/proj" PROJ_LIB)
file(TO_NATIVE_PATH ${CONFIG_PROJSHARE} GRASS_PROJSHARE)

# configure and install resource_paths.py
set(GRASS_PREFIX ${OUTDIR})
set(GISBASE_INSTALL_PATH ${GISBASE_DIR})
set(START_UP ${PROJECT_NAME_LOWER})
configure_file(app/resource_paths.py ${OUTDIR}/${PYDIR_GRASS}/app/resource_paths.py @ONLY)

set(GRASS_PREFIX ${CMAKE_INSTALL_PREFIX})
configure_file(app/resource_paths.py ${CMAKE_CURRENT_BINARY_DIR}/resource_paths.py @ONLY)
unset(GISBASE_INSTALL_PATH)
unset(GRASS_PREFIX)
unset(START_UP)

set(pydir_targets ${PYDIRS} app)
list(TRANSFORM pydir_targets REPLACE "/" "_")
list(TRANSFORM pydir_targets PREPEND "python_")

add_custom_target(
  LIB_PYTHON
  VERBATIM
  DEPENDS ${pydir_targets})

set_target_properties(LIB_PYTHON PROPERTIES FOLDER lib)

install(DIRECTORY ${OUTDIR}/${PYDIR_GRASS} DESTINATION ${GRASS_INSTALL_PYDIR}
        PATTERN "*/resource_paths.py" EXCLUDE)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/resource_paths.py
        DESTINATION ${GRASS_INSTALL_PYDIR}/grass/app)
