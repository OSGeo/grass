build_program_in_subdir(i.albedo DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)
build_program_in_subdir(i.aster.toar DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)

build_program_in_subdir(
  i.atcorr
  DEPENDS
  grass_imagery
  grass_raster
  grass_vector
  grass_gis
  SRC_REGEX
  "*.cpp")

build_program_in_subdir(i.biomass DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)
build_program_in_subdir(i.cca DEPENDS grass_imagery grass_raster grass_vector
                        grass_gis)
build_program_in_subdir(
  i.cluster
  DEPENDS
  grass_imagery
  grass_raster
  grass_vector
  grass_gis
  grass_cluster)

build_program_in_subdir(i.eb.evapfr DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)
build_program_in_subdir(i.eb.eta DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)
build_program_in_subdir(i.eb.hsebal01 DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)
build_program_in_subdir(i.eb.netrad DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)
build_program_in_subdir(i.eb.soilheatflux DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)

build_program_in_subdir(i.evapo.mh DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)
build_program_in_subdir(i.evapo.pm DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)
build_program_in_subdir(i.evapo.pt DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)
build_program_in_subdir(i.evapo.time DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)
build_program_in_subdir(i.emissivity DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)
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
  etc)

build_program_in_subdir(i.gensig DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)

build_program_in_subdir(i.gensigset DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)

build_program_in_subdir(i.group DEPENDS grass_imagery grass_raster grass_vector
                        grass_gis)

build_program_in_subdir(i.his.rgb DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)

build_program_in_subdir(i.landsat.toar DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)

build_program_in_subdir(i.maxlik DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)

build_program_in_subdir(i.modis.qc DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)

build_program_in_subdir(i.rectify DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)

build_program_in_subdir(i.rgb.his DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)

build_program_in_subdir(
  i.segment
  DEPENDS
  grass_imagery
  grass_raster
  grass_vector
  grass_gis
  grass_segment)

build_program_in_subdir(i.smap DEPENDS grass_imagery grass_raster grass_vector
                        grass_gis)

build_program_in_subdir(i.target DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)

build_program_in_subdir(i.topo.corr DEPENDS grass_imagery grass_raster
                        grass_vector grass_gis)

build_program_in_subdir(i.pca DEPENDS grass_imagery grass_raster grass_vector
                        grass_gis)

build_program_in_subdir(i.vi DEPENDS grass_imagery grass_raster grass_vector
                        grass_gis)

build_program_in_subdir(i.zc DEPENDS grass_imagery grass_raster grass_vector
                        grass_gis)

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

build_program_in_subdir(i.landsat.acca DEPENDS grass_raster grass_gis)

add_subdirectory(i.ortho.photo)
