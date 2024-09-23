build_library_in_subdir(
  lib
  NAME
  grass_iortho
  DEPENDS
  grass_gis
  grass_imagery
  grass_gmath)

build_program_in_subdir(i.ortho.camera DEPENDS grass_iortho grass_imagery
                        grass_gis grass_gmath)

build_program_in_subdir(i.ortho.elev DEPENDS grass_iortho grass_imagery
                        grass_gis grass_gmath)

build_program_in_subdir(
  i.ortho.init
  DEPENDS
  grass_iortho
  grass_imagery
  grass_gis
  grass_gmath
  grass_raster)

build_program_in_subdir(i.ortho.photo DEPENDS grass_iortho grass_imagery
                        grass_gis)

build_program_in_subdir(
  i.ortho.rectify
  DEPENDS
  grass_iortho
  grass_imagery
  grass_gis
  grass_gmath
  grass_raster
  ${LIBM})

build_program_in_subdir(i.ortho.target DEPENDS grass_iortho grass_imagery
                        grass_gis)

build_program_in_subdir(i.ortho.transform DEPENDS grass_iortho grass_imagery
                        grass_gis grass_gmath ${LIBM})
