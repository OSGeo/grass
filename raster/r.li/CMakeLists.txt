set(SUBDIRS1
    r.li.cwed
    r.li.dominance
    r.li.edgedensity
    r.li.mpa
    r.li.mps
    r.li.padcv
    r.li.padrange
    r.li.padsd
    r.li.patchdensity
    r.li.patchnum
    r.li.pielou
    r.li.renyi
    r.li.richness
    r.li.shannon
    r.li.shape
    r.li.simpson)
# r.li.setup

build_library_in_subdir(
  r.li.daemon
  NAME
  grass_rli
  DEPENDS
  grass_gis
  grass_raster
  HTML_FILE_NAME
  "r.li.daemon")

if(WITH_DOCS)
  generate_html(TARGET grass_rli NAME r.li)
endif()

foreach(SUBDIR ${SUBDIRS1})
  build_program_in_subdir(${SUBDIR} DEPENDS grass_gis grass_rli grass_raster ${LIBM})
endforeach()
