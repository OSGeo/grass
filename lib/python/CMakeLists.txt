
include(build_pymodule_in_subdir)
build_pymodule_in_subdir(exceptions)

if(WIN32)
  set(LD_LIBRARY_PATH_VAR "$ENV{LD_LIBRARY_PATH}")
else()
  set(LD_LIBRARY_PATH_VAR "$ENV{PATH}")
endif()

configure_file(script/setup.py
  ${CMAKE_BINARY_DIR}/lib/python/script/setup.py
  COPYONLY)

build_pymodule_in_subdir(script
  core.py  
  db.py
  raster.py
  raster3d.py
  vector.py
  array.py
  ${CMAKE_BINARY_DIR}/lib/python/script/setup.py
  task.py
  utils.py)

 add_subdirectory(ctypes)
 
 build_pymodule_in_subdir(temporal 
   base.py
   core.py
   abstract_dataset.py
   abstract_map_dataset.py
   abstract_space_time_dataset.py
   space_time_datasets.py
   open_stds.py
   factory.py
   gui_support.py
   list_stds.py
   register.py
   sampling.py
   metadata.py
   spatial_extent.py
   temporal_extent.py
   datetime_math.py
   temporal_granularity.py
   spatio_temporal_relationships.py
   unit_tests.py
   aggregation.py
   stds_export.py
   stds_import.py
   extract.py
   mapcalc.py
   univar_statistics.py
   temporal_topology_dataset_connector.py
   spatial_topology_dataset_connector.py
   c_libraries_interface.py
   temporal_algebra.py
   temporal_vector_algebra.py
   temporal_raster_base_algebra.py
   temporal_raster_algebra.py
   temporal_raster3d_algebra.py
   temporal_operator.py)


 build_pymodule_in_subdir(pygrass
   errors.py utils.py orderdict.py)
 
 build_pymodule_in_subdir(pygrass/messages)

 build_pymodule_in_subdir(pygrass/modules shortcuts.py)

 build_pymodule_in_subdir(pygrass/modules/interface
   docstring.py read.py typedict.py flag.py
   parameter.py module.py env.py)

 build_pymodule_in_subdir(pygrass/modules/grid
   split.py patch.py grid.py)
 
 build_pymodule_in_subdir(pygrass/raster
   abstract.py buffer.py category.py
   history.py raster_type.py rowio.py segment.py)

 build_pymodule_in_subdir(pygrass/vector
   abstract.py basic.py find.py geometry.py
   sql.py table.py vector_type.py)
 
 build_pymodule_in_subdir(pygrass/gis
   region.py)
 
 build_pymodule_in_subdir(pygrass/shell
   conversion.py show.py)

 build_pymodule_in_subdir(pygrass/tests
   benchmark.py set_mapset.py)

 build_pymodule_in_subdir(pygrass/rpc base.py)
 
 build_pymodule_in_subdir(pydispatch
   dispatcher.py errors.py robustapply.py
   robust.py saferef.py signal.py)
 
 build_pymodule_in_subdir(imaging
   images2avi.py images2gif.py
   images2ims.py images2swf.py)
 
 build_pymodule_in_subdir(gunittest
  case.py gmodules.py loader.py
  runner.py checkers.py gutils.py
  invoker.py main.py reporters.py utils.py)

install(FILES __init__.py DESTINATION etc/python/grass)
