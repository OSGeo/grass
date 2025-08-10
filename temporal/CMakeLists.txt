set(temporal_script_modules_list
    t.copy
    t.create
    t.info
    t.list
    t.merge
    t.rast.accdetect
    t.rast.accumulate
    t.rast.aggregate
    t.rast.aggregate.ds
    t.rast.algebra
    t.rast.colors
    t.rast.contour
    t.rast.export
    t.rast.extract
    t.rast.gapfill
    t.rast.import
    t.rast.list
    t.rast.mapcalc
    t.rast.neighbors
    t.rast.out.vtk
    t.rast.series
    t.rast.to.rast3
    t.rast.to.vect
    t.rast.univar
    t.rast.what
    t.rast3d.algebra
    t.rast3d.extract
    t.rast3d.list
    t.rast3d.mapcalc
    t.rast3d.univar
    t.register
    t.remove
    t.rename
    t.sample
    t.select
    t.shift
    t.snap
    t.support
    t.topology
    t.unregister
    t.upgrade
    t.vect.algebra
    t.vect.db.select
    t.vect.export
    t.vect.extract
    t.vect.import
    t.vect.list
    t.vect.observe.strds
    t.vect.univar
    t.vect.what.strds)

set(temporal_modules_list t.connect)

add_custom_target(
  ALL_TEMPORAL_MODULES ALL
  DEPENDS ${temporal_script_modules_list} ${temporal_modules_list}
  COMMENT "All temporal modules are built.")

if(WITH_DOCS)
  generate_html(TARGET ALL_TEMPORAL_MODULES NAME temporalintro)
endif()

build_program_in_subdir(t.connect DEPENDS grass_gis grass_temporal)

foreach(temporal_DIR ${temporal_script_modules_list})
  build_script_in_subdir(${temporal_DIR})
endforeach()
