function(build_pyfiles_in_sudir gpy_dir_name)
  #  cmake_parse_arguments(GPY  "" "NAME" "FILES" ${ARGN} )
  set( gpy_files ${ARGN} )
  if (NOT "__init__.py" IN_LIST gpy_files)
    list(APPEND gpy_files "__init__.py")
  endif()
  
  get_filename_component(gpy_name ${gpy_dir_name} NAME)
  string(REPLACE "/" ";" input_list "${gpy_dir_name}")

  list(LENGTH input_list input_list_LEN)  
  if(input_list_LEN EQUAL 1)
    set(dest_dir ${gpy_dir_name})
  else()
    list(GET input_list 0 dest_dir)
  endif()

  foreach(gpy_file ${gpy_files})    
    if(NOT IS_ABSOLUTE ${gpy_file})
      #message("gpy_file=${gpy_dir_name}/${gpy_file}")
      get_filename_component(gpy_file ${gpy_dir_name}/${gpy_file} REALPATH)
    endif()
    if(NOT EXISTS ${gpy_file})
      message(FATAL_ERROR "Cannot find '${gpy_file}'")
    endif()
    get_filename_component(gpy_file_NAME ${gpy_file} NAME)
    file(COPY ${gpy_file} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${gpy_dir_name})
    # <----- RESET HERE ----->
    set(gpy_file ${CMAKE_CURRENT_BINARY_DIR}/${gpy_dir_name}/${gpy_file_NAME})
    # <----- RESET HERE ----->


    add_custom_command(
      OUTPUT ${gpy_file}.stamp
      COMMAND ${PYTHON_EXECUTABLE} -t -m py_compile ${gpy_file}
      COMMAND ${CMAKE_COMMAND} -E touch  ${gpy_file}.stamp
      COMMENT "Generating ${gpy_file}c"
      VERBATIM
      )
  
    add_custom_target(${gpy_name}.${gpy_file_NAME} ALL
      DEPENDS ${gpy_file}.stamp
      )
    
    # add_custom_target(${gpy_name}_${gpy_file_NAME} ALL
    #   COMMAND ${PYTHON_EXECUTABLE} -t -3 -m py_compile ${gpy_file}
    #   COMMENT "Generating ${gpy_file}c"
    #   )
    
    #set_source_files_properties("${gpy_file}c" GENERATED)

    #message(" ${gpy_dir_name} | DESTINATION  etc/python/${gpy_dir_name} ")
    install(FILES ${gpy_file} DESTINATION  etc/python/grass/${dest_dir} )
    
  endforeach()
endfunction()
   
build_pyfiles_in_sudir(exceptions)

set(LD_LIBRARY_PATH_VAR "$ENV{LD_LIBRARY_PATH}")

configure_file(  script/setup.py
  ${CMAKE_BINARY_DIR}/lib/python/script/setup.py
  COPYONLY)

build_pyfiles_in_sudir(script
  core.py  
  db.py
  raster.py
  raster3d.py
  vector.py
  array.py
  ${CMAKE_BINARY_DIR}/lib/python/script/setup.py
  task.py
  utils.py
 )

  

# build_pyfiles_in_sudir(NAME ctypes
#   FILES
#   date.py gis.py  raster.py  gmath.py  proj.py
#   imagery.py  vector.py rtree.py  display.py
#   stats.py dbmi.py raster3d.py arraystats.py
#   cluster.py vedit.py segment.py rowio.py
#   temporal.py
#   ctypes_preamble.py ctypes_loader.py
#  )

add_subdirectory(ctypes)


 build_pyfiles_in_sudir(temporal 
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
   temporal_operator.py   
   )


 build_pyfiles_in_sudir(pygrass
   errors.py utils.py orderdict.py
   )
 
 build_pyfiles_in_sudir(pygrass/messages )

 build_pyfiles_in_sudir(pygrass/modules
   shortcuts.py )

 build_pyfiles_in_sudir(pygrass/modules/interface
   docstring.py read.py typedict.py flag.py
   parameter.py module.py env.py
   )

 build_pyfiles_in_sudir(pygrass/modules/grid
   split.py patch.py grid.py )
 
 build_pyfiles_in_sudir(pygrass/raster
   abstract.py buffer.py category.py
   history.py raster_type.py rowio.py segment.py
   )

 build_pyfiles_in_sudir(pygrass/vector
   abstract.py basic.py find.py geometry.py
   sql.py table.py vector_type.py
   )
 
 build_pyfiles_in_sudir(pygrass/gis
   region.py
   )
 
 build_pyfiles_in_sudir(pygrass/shell
   conversion.py show.py
   )

 build_pyfiles_in_sudir(pygrass/tests
   benchmark.py set_mapset.py
   )

 build_pyfiles_in_sudir(pygrass/rpc
   base.py
   )
 
 build_pyfiles_in_sudir(pydispatch
   dispatcher.py errors.py robustapply.py
   robust.py saferef.py signal.py
   )
 
 build_pyfiles_in_sudir(imaging
   images2avi.py images2gif.py
   images2ims.py images2swf.py)
 
 build_pyfiles_in_sudir(gunittest
  case.py gmodules.py loader.py
  runner.py checkers.py gutils.py
  invoker.py main.py reporters.py utils.py)
