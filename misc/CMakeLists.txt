
build_program_in_subdir(
  m.cogo
  DEPENDS grass_gis)

build_program_in_subdir(
  m.measure
  DEPENDS grass_gis)

build_program_in_subdir(
  m.transform
  DEPENDS grass_gis grass_imagery grass_raster grass_vector)

# build_program_in_subdir(
#   m.nviz.image
#   DEPENDS grass_gis)

# build_program_in_subdir(
#   m.nviz.script
#   DEPENDS grass_gis)
