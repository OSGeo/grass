file(GLOB SQLFILES "SQL/*.sql")
foreach(sqlfile ${SQLFILES})
  install(FILES "${sqlfile}" DESTINATION ${GRASS_INSTALL_ETCDIR}/sql)
endforeach()
build_library_in_subdir(
  lib
  NAME
  grass_temporal
  DEPENDS
  grass_gis
  grass_dbmibase
  grass_datetime)
