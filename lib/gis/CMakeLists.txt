file(GLOB gislib_SRCS "*.c")

if(NOT WIN32)
    list(FILTER gislib_SRCS EXCLUDE REGEX [[.*/fmode\.c$]])
endif()

set(grass_gis_DEFS "-DGRASS_VERSION_DATE=\"${GRASS_VERSION_DATE}\"")
if(MSVC)
    set(grass_gis_DEFS "${grass_gis_DEFS};-D_USE_MATH_DEFINES=1")
    set(gislib_INCLUDES "${PROJECT_SOURCE_DIR}/msvc")
    list(
        APPEND gislib_SRCS
        "${PROJECT_SOURCE_DIR}/msvc/dirent.c;${PROJECT_SOURCE_DIR}/msvc/fcntl.c"
    )
endif()

build_module(
    NAME grass_gis
    INCLUDES ${gislib_INCLUDES}
    SOURCES "${gislib_SRCS}"
    DEPENDS PROJ::proj ZLIB::ZLIB grass_datetime
    OPTIONAL_DEPENDS
        BZip2::BZip2
        Iconv::Iconv
        Intl::Intl
        OpenMP::OpenMP_C
        PostgreSQL::PostgreSQL
        Threads::Threads
        ZSTD
    DEFS "${grass_gis_DEFS}"
)

add_custom_command(
    TARGET grass_gis
    POST_BUILD
    COMMAND
        ${CMAKE_COMMAND} -E make_directory
        ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/proj
    COMMAND
        ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/colors
        ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/colors
    COMMAND
        ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/ellipse.table
        ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/proj
    COMMAND
        ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/datum.table
        ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/proj
    COMMAND
        ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/datumtransform.table
        ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/proj
    COMMAND
        ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/FIPS.code
        ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/proj
    COMMAND
        ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/state27
        ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/proj
    COMMAND
        ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/state83
        ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/proj
    COMMAND
        ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/projections
        ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/proj
    COMMAND
        ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/ellipse.table.solar.system
        ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/proj
    COMMAND
        ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/colors.desc
        ${OUTDIR}/${GRASS_INSTALL_ETCDIR}
    COMMAND
        ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/renamed_options
        ${OUTDIR}/${GRASS_INSTALL_ETCDIR}
)
install(DIRECTORY colors DESTINATION ${GRASS_INSTALL_ETCDIR})
install(
    FILES
        ellipse.table
        ellipse.table.solar.system
        datum.table
        datumtransform.table
        FIPS.code
        state27
        state83
        projections
    DESTINATION ${GRASS_INSTALL_ETCDIR}/proj
)
install(FILES colors.desc renamed_options DESTINATION ${GRASS_INSTALL_ETCDIR})

target_include_directories(
    grass_gis
    INTERFACE $<TARGET_PROPERTY:ZLIB::ZLIB,INTERFACE_INCLUDE_DIRECTORIES>
)

if(UNIX)
    target_link_libraries(grass_gis PRIVATE ${LIBM})
endif()

if(MSVC)
    target_link_libraries(grass_gis PRIVATE PCRE)
endif()

target_link_libraries(grass_gis PRIVATE ${CMAKE_DL_LIBS})

# configure and install grass.conf
set(GISBASE_INSTALL_PATH ${GISBASE_DIR})
set(START_UP ${PROJECT_NAME_LOWER})
set(GRASS_PREFIX_RELATIVE
    "$<PATH:RELATIVE_PATH,${OUTDIR},$<TARGET_FILE_DIR:grass_gis>/grass>"
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/resource_dirs.conf.in"
    "${CMAKE_CURRENT_BINARY_DIR}/resource_dirs.conf.gen"
    @ONLY
)

file(
    GENERATE OUTPUT "$<TARGET_FILE_DIR:grass_gis>/grass/grass.conf"
    INPUT "${CMAKE_CURRENT_BINARY_DIR}/resource_dirs.conf.gen"
)

cmake_path(
    RELATIVE_PATH OUTDIR
    BASE_DIRECTORY "${OUTDIR}/${GRASS_INSTALL_LIBDIR}/grass"
    OUTPUT_VARIABLE GRASS_PREFIX_RELATIVE
)

configure_file(
    resource_dirs.conf.in
    ${CMAKE_CURRENT_BINARY_DIR}/grass.conf
    @ONLY
)

unset(GISBASE_INSTALL_PATH)
unset(START_UP)
unset(GRASS_PREFIX_RELATIVE)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/grass.conf
    DESTINATION ${GRASS_INSTALL_LIBDIR}/grass
)
