set(_srcfiles
    WIND
    DEFAULT_WIND
    VAR
    PROJ_INFO
    PROJ_UNITS
    PROJ_SRID
    PROJ_EPSG
    MYNAME)
set(_vectfiles
    dbln
    coor
    topo
    head
    cidx
    sidx
    hist)
set(_sqlitefiles sqlite/sqlite.db)
set(_sqlitemaps country_boundaries)

list(TRANSFORM _vectfiles REPLACE "(.+)" "vector/${_sqlitemaps}/\\1")
set(_src ${_srcfiles} ${_sqlitefiles} ${_vectfiles})
list(TRANSFORM _src PREPEND "PERMANENT/")
set(_srcdirs PERMANENT PERMANENT/vector PERMANENT/sqlite
             PERMANENT/vector/${_sqlitemaps})
set(_srcdirs_out ${_srcdirs})
list(TRANSFORM _srcdirs_out PREPEND "${OUTDIR}/${GRASS_INSTALL_DEMODIR}/")

file(MAKE_DIRECTORY ${_srcdirs_out})

foreach(_s ${_src})
  get_filename_component(_s_dir ${_s} DIRECTORY)
  file(COPY ${_s} DESTINATION ${OUTDIR}/${GRASS_INSTALL_DEMODIR}/${_s_dir})
  install(FILES ${OUTDIR}/${GRASS_INSTALL_DEMODIR}/${_s}
          DESTINATION ${GRASS_INSTALL_DEMODIR}/${_s_dir})
endforeach()

set(GISDBASE ${RUNTIME_GISBASE})
configure_file(${CMAKE_SOURCE_DIR}/demolocation/grassrc.tmpl "${GISRC}" @ONLY)
set(GISDBASE ${GISBASE})
configure_file(${CMAKE_SOURCE_DIR}/demolocation/grassrc.tmpl
               "${CMAKE_CURRENT_BINARY_DIR}/${GISRC_NAME}" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${GISRC_NAME}"
        DESTINATION "${GRASS_INSTALL_DEMODIR}")
unset(GISDBASE)
unset(_srcfiles)
unset(_vectfiles)
unset(_sqlitefiles)
unset(_sqlitemaps)
unset(_src)
unset(_srcdirs)
unset(_srcdirs_out)
