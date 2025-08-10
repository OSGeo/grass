set(r_univar_SRCS r.univar_main.c sort.c stats.c)
set(r3_univar_SRCS r3.univar_main.c sort.c stats.c)

build_program(
  NAME
  r.univar
  SOURCES
  "${r_univar_SRCS}"
  DEPENDS
  grass_gis
  grass_raster
  grass_parson
  ${LIBM}
  OPTIONAL_DEPENDS
  OPENMP)

build_program(
  NAME
  r3.univar
  SOURCES
  "${r3_univar_SRCS}"
  DEPENDS
  grass_gis
  grass_raster
  grass_raster3d
  grass_parson
  ${LIBM}
  OPTIONAL_DEPENDS
  OPENMP)
