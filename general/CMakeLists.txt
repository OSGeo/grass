build_program_in_subdir(g.access DEPENDS grass_gis)
build_program_in_subdir(g.copy DEPENDS grass_gis grass_manage)
build_program_in_subdir(g.dirseps DEPENDS grass_gis)
build_program_in_subdir(g.filename DEPENDS grass_gis)
build_program_in_subdir(g.findetc DEPENDS grass_gis)
build_program_in_subdir(g.findfile DEPENDS grass_gis grass_manage)
build_program_in_subdir(g.gisenv DEPENDS grass_gis)
build_program_in_subdir(g.mapset DEPENDS grass_gis)
build_program_in_subdir(g.mapsets DEPENDS grass_gis grass_parson)
build_program_in_subdir(g.message DEPENDS grass_gis)
build_program_in_subdir(g.mkfontcap DEPENDS grass_gis PRIMARY_DEPENDS
                        Freetype::Freetype)
add_dependencies(g.mkfontcap fonts)
add_custom_command(
  TARGET g.mkfontcap
  POST_BUILD
  COMMAND ${grass_env_command} ${OUTDIR}/${GRASS_INSTALL_BINDIR}/g.mkfontcap -s
          > ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/fontcap)
install(FILES ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/fontcap
        DESTINATION ${GRASS_INSTALL_ETCDIR})
build_program_in_subdir(g.parser DEPENDS grass_gis)
build_program_in_subdir(g.pnmcomp DEPENDS grass_gis)
build_program_in_subdir(g.ppmtopng DEPENDS grass_gis PRIMARY_DEPENDS PNG::PNG)
build_program_in_subdir(
  g.proj
  DEPENDS
  grass_gis
  grass_gproj
  grass_parson
  PROJ::proj
  OPTIONAL_DEPENDS
  GDAL::GDAL)

build_program_in_subdir(
  g.region
  DEPENDS
  grass_gis
  grass_gproj
  grass_vector
  grass_raster
  grass_raster3d
  grass_gmath
  grass_parson
  PROJ::proj
  ${LIBM})

build_program_in_subdir(g.rename DEPENDS grass_gis grass_manage grass_raster)
build_program_in_subdir(g.tempfile DEPENDS grass_gis)

build_program_in_subdir(
  g.version
  DEPENDS
  grass_gis
  OPTIONAL_DEPENDS
  GDAL::GDAL
  GEOS::geos_c
  PROJ::proj
  SQLite::SQLite3
  DEFS
  "-DGRASS_VERSION_NUMBER=\"${GRASS_VERSION_NUMBER}\""
  "-DGRASS_VERSION_GIT=\"${GRASS_VERSION_GIT}\""
  "-DGRASS_VERSION_UPDATE_PKG=${GRASS_VERSION_UPDATE_PKG}"
  "-DARCH=\"${BUILD_ARCH}\"")

if(WITH_CAIRO
   AND WITH_X11
   AND CAIRO_HAS_XLIB_XRENDER_SURFACE)
  if(NOT MSVC)
    build_program_in_subdir(
      g.cairocomp
      DEPENDS
      grass_gis
      Fontconfig::Fontconfig
      X11::X11
      PRIMARY_DEPENDS
      Cairo::Cairo)
  endif()
endif()

add_subdirectory(manage/lister)

build_program_in_subdir(
  g.list
  DEPENDS
  grass_gis
  grass_manage
  grass_raster
  grass_raster3d
  grass_vector)
add_dependencies(g.list cell vector)

build_program_in_subdir(g.remove DEPENDS grass_gis grass_manage grass_raster)

build_program_in_subdir(g.gui DEPENDS grass_raster grass_gis)
