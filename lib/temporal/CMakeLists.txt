file(GLOB SQLFILES "SQL/*.sql")
install(FILES "${SQLFILES}" DESTINATION etc/sql)

build_library_in_subdir(lib NAME grass_temporal DEPENDS grass_dbmibase grass_datetime)
