set(PYDIRS
    app
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

configure_file(__init__.py ${OUTDIR}/${PYDIR_GRASS}/ COPYONLY)
configure_file(script/setup.py ${OUTDIR}/${PYDIR_GRASS}/script/setup.py
               COPYONLY)

set(pydir_targets ${PYDIRS})
list(TRANSFORM pydir_targets REPLACE "/" "_")
list(TRANSFORM pydir_targets PREPEND "python_")

add_custom_target(
  LIB_PYTHON
  VERBATIM
  DEPENDS ${pydir_targets})

set_target_properties(LIB_PYTHON PROPERTIES FOLDER lib)

install(DIRECTORY ${OUTDIR}/${PYDIR_GRASS} DESTINATION ${GRASS_INSTALL_PYDIR})
