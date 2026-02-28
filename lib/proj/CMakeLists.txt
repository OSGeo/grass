install(FILES desc.table parms.table units.table
        DESTINATION ${GRASS_INSTALL_ETCDIR}/proj)

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
  "${CMAKE_CURRENT_SOURCE_DIR}/../driver")

target_include_directories(grass_gproj
  INTERFACE $<TARGET_PROPERTY:GDAL::GDAL,INTERFACE_INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:PROJ::proj,INTERFACE_INCLUDE_DIRECTORIES>)
