set(imagery_modules_list
    i.albedo
    i.aster.toar
    i.atcorr
    i.biomass
    i.cca
    i.cluster
    i.eb.eta
    i.eb.evapfr
    i.eb.hsebal01
    i.eb.netrad
    i.eb.soilheatflux
    i.emissivity
    i.evapo.mh
    i.evapo.pm
    i.evapo.pt
    i.evapo.time
    i.fft
    i.find
    i.gensig
    i.gensigset
    i.group
    i.his.rgb
    i.ifft
    i.landsat.acca
    i.landsat.toar
    i.maxlik
    i.modis.qc
    i.pca
    i.rectify
    i.rgb.his
    i.segment
    i.signatures
    i.smap
    i.target
    i.topo.corr
    i.vi
    i.zc)

# TODO: add i.svm.predict and i.svm.train

add_custom_target(
  ALL_IMAGERY_MODULES ALL
  DEPENDS ${imagery_modules_list}
  COMMENT "All imagery modules are built.")

if(WITH_DOCS)
  generate_html(TARGET ALL_IMAGERY_MODULES NAME imageryintro)

  file(COPY ${CMAKE_SOURCE_DIR}/lib/imagery/band_references_scheme.png
       DESTINATION ${OUTDIR}/${GRASS_INSTALL_DOCDIR})
  install(FILES ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/band_references_scheme.png
          DESTINATION ${GRASS_INSTALL_DOCDIR})
endif()

build_program_in_subdir(i.albedo DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)
build_program_in_subdir(i.aster.toar DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis ${LIBM})

build_program_in_subdir(
  i.atcorr
  DEPENDS
  grass_imagery
  grass_raster
  grass_vector
  grass_gis
  grass_btree2
  ${LIBM}
  SRC_REGEX
  "*.cpp")

build_program_in_subdir(i.biomass DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis ${LIBM})
build_program_in_subdir(
  i.cca
  DEPENDS
  grass_imagery
  grass_raster
  grass_vector
  grass_gis
  grass_gmath
  ${LIBM})
build_program_in_subdir(
  i.cluster
  DEPENDS
  grass_imagery
  grass_raster
  grass_vector
  grass_gis
  grass_cluster)

build_program_in_subdir(i.eb.evapfr DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis ${LIBM})
build_program_in_subdir(i.eb.eta DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis ${LIBM})
build_program_in_subdir(i.eb.hsebal01 DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis ${LIBM})
build_program_in_subdir(i.eb.netrad DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis ${LIBM})
build_program_in_subdir(i.eb.soilheatflux DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis ${LIBM})

build_program_in_subdir(i.evapo.mh DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis ${LIBM})
build_program_in_subdir(i.evapo.pm DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis ${LIBM})
build_program_in_subdir(i.evapo.pt DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis ${LIBM})
build_program_in_subdir(i.evapo.time DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)
build_program_in_subdir(i.emissivity DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis ${LIBM})
build_program_in_subdir(
  i.find
  DEPENDS
  grass_imagery
  grass_raster
  grass_vector
  grass_gis
  PACKAGE
  "grassmods"
  RUNTIME_OUTPUT_DIR
  ${GRASS_INSTALL_ETCDIR}
  NO_DOC)

build_program_in_subdir(i.gensig DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis ${LIBM})

build_program_in_subdir(
  i.gensigset
  DEPENDS
  grass_imagery
  grass_raster
  grass_vector
  grass_gis
  grass_gmath
  ${LIBM})

build_program_in_subdir(i.group DEPENDS grass_imagery grass_raster grass_vector
                        grass_gis)

build_program_in_subdir(i.his.rgb DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)

build_program_in_subdir(i.landsat.toar DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis ${LIBM})

build_program_in_subdir(i.maxlik DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis ${LIBM})

build_program_in_subdir(i.modis.qc DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)

build_program_in_subdir(i.rectify DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis ${LIBM})

build_program_in_subdir(i.rgb.his DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)

build_program_in_subdir(
  i.segment
  DEPENDS
  grass_imagery
  grass_raster
  grass_vector
  grass_gis
  grass_segment
  ${LIBM})

build_program_in_subdir(i.signatures DEPENDS grass_imagery grass_gis)

build_program_in_subdir(
  i.smap
  DEPENDS
  grass_imagery
  grass_raster
  grass_vector
  grass_gis
  grass_gmath
  ${LIBM})

build_program_in_subdir(i.target DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)

build_program_in_subdir(i.topo.corr DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis ${LIBM})

build_program_in_subdir(
  i.pca
  DEPENDS
  grass_imagery
  grass_raster
  grass_vector
  grass_gis
  grass_gmath
  ${LIBM})

build_program_in_subdir(i.vi DEPENDS grass_imagery grass_raster grass_vector
                        grass_gis ${LIBM})

build_program_in_subdir(
  i.zc
  DEPENDS
  grass_imagery
  grass_raster
  grass_vector
  grass_gis
  grass_gmath)

build_program_in_subdir(
  i.fft
  DEPENDS
  grass_imagery
  grass_raster
  grass_gmath
  grass_gis
  PRIMARY_DEPENDS
  FFTW)

build_program_in_subdir(
  i.ifft
  DEPENDS
  grass_gis
  grass_raster
  grass_gmath
  grass_btree2
  PRIMARY_DEPENDS
  FFTW)

build_program_in_subdir(i.landsat.acca DEPENDS grass_raster grass_gis ${LIBM})

# TODO: add generate docs for i.ortho.photo
add_subdirectory(i.ortho.photo)
