set(script_DIRS
  r.shade
  d.correlate
  d.frame
  d.out.file
  d.to.rast
  d.polar
  d.rast.edit
  d.rast.leg
  d.redraw
  d.shade
  d.what.rast
  d.what.vect
  db.dropcolumn
  db.droptable
  db.in.ogr
  db.out.ogr
  db.test
  db.univar
  g.extension
  g.extension.all
  g.manual
  g.search.modules
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
  r.fillnulls
  r.grow
  r.import
  r.in.aster
  r.in.srtm
  r.mask
  r.out.xyz
  r.pack
  r.plane
  r.reclass.area
  r.rgb
  r.tileset
  r.unpack
  r3.in.xyz
  v.build.all
  v.centroids
  v.clip
  v.db.addcolumn
  v.db.addtable
  v.db.join
  v.db.dropcolumn
  v.db.droprow
  v.db.droptable
  v.db.renamecolumn
  v.db.reconnect.all
  v.db.univar
  v.db.update
  v.dissolve
  v.import
  v.in.e00
  v.in.geonames
  v.in.lines
  v.in.mapgen
  v.in.wfs
  v.rast.stats
  v.report
  v.pack
  v.to.lines
  v.unpack
  v.what.strds
  v.what.vect
  wxpyimgview
  r.mapcalc.simple
  r.in.wms
  )

foreach(script_DIR ${script_DIRS})
  build_script_in_subdir(${script_DIR})
endforeach()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/r.in.wms
  DESTINATION ${CMAKE_BINARY_DIR}/etc/)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/wxpyimgview/wxpyimgview_gui.py
  DESTINATION ${CMAKE_BINARY_DIR}/etc/)

add_custom_target(r.in.wms_files
	COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/etc/r.in.wms/
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/r.in.wms/wms_base.py ${GISBASE}/etc/r.in.wms/
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/r.in.wms/wms_cap_parsers.py ${GISBASE}/etc/r.in.wms/
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/r.in.wms/wms_drv.py ${GISBASE}/etc/r.in.wms/
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/r.in.wms/wms_gdal_drv.py ${GISBASE}/etc/r.in.wms/
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/r.in.wms/srs.py ${GISBASE}/etc/r.in.wms/
	DEPENDS v.to.lines
	)
add_dependencies(r.in.wms r.in.wms_files)

 set_target_properties (r.in.wms_files PROPERTIES FOLDER scripts)