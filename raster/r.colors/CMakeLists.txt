set(r_colors_SRCS raster_main.c edit_colors.c rules.c stats.c)
set(r3_colors_SRCS raster3d_main.c edit_colors.c rules.c stats.c)

build_program(
  NAME
  r.colors
  SOURCES
  "${r_colors_SRCS}"
  DEPENDS
  grass_gis
  grass_raster
  grass_raster3d
  ${LIBM})

build_program(
  NAME
  r3.colors
  SOURCES
  "${r3_colors_SRCS}"
  DEPENDS
  grass_gis
  grass_raster
  grass_raster3d
  ${LIBM})
