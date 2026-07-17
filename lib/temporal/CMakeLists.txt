file(GLOB SQLFILES "SQL/*.sql")
foreach(sqlfile ${SQLFILES})
    install(FILES "${sqlfile}" DESTINATION ${GRASS_INSTALL_ETCDIR}/sql)
endforeach()
build_library_in_subdir(
    lib
    NAME grass_temporal
    DEPENDS grass_datetime grass_dbmibase grass_gis
)

add_custom_command(
    TARGET grass_temporal
    POST_BUILD
    COMMAND
        ${CMAKE_COMMAND} -E make_directory ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/sql
    COMMAND
        ${CMAKE_COMMAND} -E copy ${SQLFILES}
        ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/sql
)
