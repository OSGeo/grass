

make_script_in_dir(r.shade)

make_script_in_dir(d.correlate)

make_script_in_dir(d.frame)

make_script_in_dir(d.out.file )

set(scripts_SUBDIRS
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
  
  )

  #TODO
  #r.in.wms
  #wxpyimgview
  make_script_in_dir( r.in.wms )
  
foreach(d ${scripts_SUBDIRS})
make_script_in_dir(${d} )
endforeach()


