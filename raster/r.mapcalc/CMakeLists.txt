flex_target(mapcalc.yy.c mapcalc.l ${CMAKE_CURRENT_BINARY_DIR}/mapcalc.yy.c)
bison_target(mapcalc.tab.c mapcalc.y ${CMAKE_CURRENT_BINARY_DIR}/mapcalc.tab.c)
add_flex_bison_dependency(mapcalc.yy.c mapcalc.tab.c)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/mapcalc.yy.c GENERATED)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/mapcalc.tab.c GENERATED)

set(r_mapcalc_common_SRCS
    column_shift.c
    evaluate.c
    expression.c
    function.c
    main.c
    xrowcol.c
    ${CMAKE_CURRENT_BINARY_DIR}/mapcalc.tab.c
    ${CMAKE_CURRENT_BINARY_DIR}/mapcalc.yy.c)

set(r_mapcalc_SRCS ${r_mapcalc_common_SRCS} map.c xarea.c xcoor.c xres.c)
set(r3_mapcalc_SRCS ${r_mapcalc_common_SRCS} map3.c xarea.c xcoor3.c xres3.c)

build_program(
  NAME
  r.mapcalc
  SOURCES
  "${r_mapcalc_SRCS}"
  DEPENDS
  grass_btree
  grass_calc
  grass_gis
  grass_raster
  OPTIONAL_DEPENDS
  Readline::Readline
  Readline::History
  Threads::Threads)

build_program(
  NAME
  r3.mapcalc
  SOURCES
  "${r3_mapcalc_SRCS}"
  DEPENDS
  grass_btree
  grass_calc
  grass_gis
  grass_raster
  grass_raster3d
  OPTIONAL_DEPENDS
  Readline::Readline
  Readline::History
  Threads::Threads)
