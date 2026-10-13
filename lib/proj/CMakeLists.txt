install(
    FILES desc.table parms.table units.table
    DESTINATION ${GRASS_INSTALL_ETCDIR}/proj
)

set(grass_gproj_SOURCES
    convert.c
    datum.c
    do_proj.c
    ellipse.c
    get_proj.c
)
if(MINGW)
    list(APPEND grass_gproj_SOURCES "ftol.c")
endif()

build_module(
    NAME grass_gproj
    DEPENDS GDAL::GDAL PROJ::proj grass_driver grass_gis
    SOURCES "${grass_gproj_SOURCES}"
    INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/../driver"
)

add_custom_command(
    TARGET grass_gproj
    POST_BUILD
    COMMAND
        ${CMAKE_COMMAND} -E make_directory
        ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/proj
    COMMAND
        ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/desc.table
        ${CMAKE_CURRENT_SOURCE_DIR}/parms.table
        ${CMAKE_CURRENT_SOURCE_DIR}/units.table
        ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/proj
)

target_include_directories(
    grass_gproj
    INTERFACE
        $<TARGET_PROPERTY:GDAL::GDAL,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:PROJ::proj,INTERFACE_INCLUDE_DIRECTORIES>
)
