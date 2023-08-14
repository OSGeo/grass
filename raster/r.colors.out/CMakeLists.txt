set(r_colors_out_SRCS raster_main.c)
set(r3_colors_out_SRCS raster3d_main.c)

build_program(
  NAME
  r.colors.out
  SOURCES
  "${r_colors_out_SRCS}"
  DEPENDS
  grass_gis
  grass_raster)

build_program(
  NAME
  r3.colors.out
  SOURCES
  "${r3_colors_out_SRCS}"
  DEPENDS
  grass_gis
  grass_raster3d)
