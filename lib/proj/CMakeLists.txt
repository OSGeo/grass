file(COPY desc.table DESTINATION etc/proj)
file(COPY parms.table DESTINATION etc/proj)
file(COPY units.table DESTINATION etc/proj)

set(grass_gproj_SOURCES convert.c datum.c do_proj.c ellipse.c get_proj.c)
if(MINGW)
  list(APPEND grass_gproj_SOURCES "ftol.c")
endif()

build_module(
  NAME
  grass_gproj
  DEPENDS
  grass_driver
  grass_gis
  GDAL::GDAL
  PROJ::proj
  SOURCES
  "${grass_gproj_SOURCES}"
  INCLUDES
  "../driver")

target_include_directories(grass_gproj PUBLIC ${GDAL_INCLUDE_DIR} ${PROJ_INCLUDE_DIRS})
