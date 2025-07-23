set(script_DIRS
    d.background
    d.correlate
    d.frame
    d.out.file
    d.polar
    d.rast.edit
    d.rast.leg
    d.shade
    d.to.rast
    d.what.rast
    d.what.vect
    db.dropcolumn
    db.droptable
    db.in.ogr
    db.out.ogr
    db.test
    db.univar
    g.download.location
    g.download.project
    g.extension
    g.extension.all
    g.manual
    g.search.modules
    i.band.library
    i.colors.enhance
    i.image.mosaic
    i.in.spotvgt
    i.oif
    i.pansharpen
    i.spectral
    i.tasscap
    m.proj
    r.blend
    r.buffer.lowmem
    r.colors.stddev
    r.drain
    r.fillnulls
    r.grow
    r.import
    r.in.aster
    r.in.srtm
    r.in.wms
    r.mapcalc.simple
    r.mask
    r.out.xyz
    r.pack
    r.plane
    r.reclass.area
    r.rgb
    r.semantic.label
    r.shade
    r.tileset
    r.unpack
    r3.in.xyz
    v.build.all
    v.centroids
    v.clip
    v.db.addcolumn
    v.db.addtable
    v.db.dropcolumn
    v.db.droprow
    v.db.droptable
    v.db.join
    v.db.reconnect.all
    v.db.renamecolumn
    v.db.univar
    v.db.update
    v.dissolve
    v.import
    v.in.e00
    v.in.geonames
    v.in.lines
    v.in.mapgen
    v.in.wfs
    v.pack
    v.rast.stats
    v.report
    v.to.lines
    v.unpack
    v.what.strds
    v.what.vect
    wxpyimgview)

foreach(script_DIR ${script_DIRS})
  build_script_in_subdir(${script_DIR})
endforeach()

add_custom_command(
  TARGET d.polar
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory
          ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/d.polar
  COMMAND
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/d.polar/ps_defs.eps
    ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/d.polar)
install(DIRECTORY ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/d.polar
        DESTINATION ${GRASS_INSTALL_ETCDIR})

add_custom_command(
  TARGET db.test
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory
          ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/db.test
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/db.test/test1
          ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/db.test)
install(DIRECTORY ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/db.test
        DESTINATION ${GRASS_INSTALL_ETCDIR})

add_custom_command(
  TARGET i.band.library
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory
          ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/i.band.library
  COMMAND
    ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/i.band.library/landsat.json
    ${CMAKE_CURRENT_SOURCE_DIR}/i.band.library/sentinel.json
    ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/i.band.library)
install(DIRECTORY ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/i.band.library
        DESTINATION ${GRASS_INSTALL_ETCDIR})

# TODO: Create CMakeLists.txt in scripts/r.in.wms and scripts/wxpyimgview
# respectively, and put the following there.

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/r.in.wms
     DESTINATION ${OUTDIR}/${GRASS_INSTALL_ETCBINDIR})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/wxpyimgview/wxpyimgview_gui.py
     DESTINATION ${OUTDIR}/${GRASS_INSTALL_ETCBINDIR})

add_custom_target(
  r.in.wms_files
  COMMAND ${CMAKE_COMMAND} -E make_directory
          ${OUTDIR}/${GRASS_INSTALL_ETCBINDIR}/r.in.wms
  COMMAND
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/r.in.wms/wms_base.py
    ${OUTDIR}/${GRASS_INSTALL_ETCBINDIR}/r.in.wms
  COMMAND
    ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/r.in.wms/wms_cap_parsers.py
    ${OUTDIR}/${GRASS_INSTALL_ETCBINDIR}/r.in.wms
  COMMAND
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/r.in.wms/wms_drv.py
    ${OUTDIR}/${GRASS_INSTALL_ETCBINDIR}/r.in.wms
  COMMAND
    ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/r.in.wms/wms_gdal_drv.py
    ${OUTDIR}/${GRASS_INSTALL_ETCBINDIR}/r.in.wms
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/r.in.wms/srs.py
          ${OUTDIR}/${GRASS_INSTALL_ETCBINDIR}/r.in.wms
  DEPENDS v.to.lines)
add_dependencies(r.in.wms r.in.wms_files)

set_target_properties(r.in.wms_files PROPERTIES FOLDER scripts)
