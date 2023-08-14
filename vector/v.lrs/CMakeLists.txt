build_library_in_subdir(
  lib
  NAME
  grass_lrs
  DEPENDS
  grass_gis
  grass_vector
  HEADERS
  "lrs.h")
add_dependencies(grass_lrs db.drivers)
build_program_in_subdir(v.lrs.create DEPENDS grass_lrs)

build_program_in_subdir(v.lrs.segment DEPENDS grass_lrs)

build_program_in_subdir(v.lrs.label DEPENDS grass_lrs)

build_program_in_subdir(v.lrs.where DEPENDS grass_lrs)
