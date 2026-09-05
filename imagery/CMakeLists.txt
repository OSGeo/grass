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
)

if(WITH_FFTW)
    list(APPEND imagery_modules_list i.zc)
endif()

if(WITH_LIBSVM)
    build_program_in_subdir(
        i.svm.predict
        DEPENDS LibSVM::LibSVM grass_gis grass_imagery grass_raster
    )

    build_program_in_subdir(
        i.svm.train
        DEPENDS LibSVM::LibSVM grass_gis grass_imagery grass_raster grass_vector
    )

    list(APPEND imagery_modules_list i.svm.predict i.svm.train)
endif()

add_custom_target(
    ALL_IMAGERY_MODULES
    ALL
    DEPENDS ${imagery_modules_list}
    COMMENT "All imagery modules are built."
)

set_target_properties(ALL_IMAGERY_MODULES PROPERTIES FOLDER Misc)

if(WITH_DOCS)
    generate_docs(imageryintro TARGET ALL_IMAGERY_MODULES)

    file(
        COPY ${CMAKE_SOURCE_DIR}/lib/imagery/band_references_scheme.png
        DESTINATION ${OUTDIR}/${GRASS_INSTALL_DOCDIR}
    )
    install(
        FILES ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/band_references_scheme.png
        DESTINATION ${GRASS_INSTALL_DOCDIR}
    )
endif()

build_program_in_subdir(
    i.albedo
    DEPENDS grass_gis grass_imagery grass_raster grass_vector
)
build_program_in_subdir(
    i.aster.toar
    DEPENDS ${LIBM} grass_gis grass_imagery grass_raster grass_vector
)

build_program_in_subdir(
    i.atcorr
    DEPENDS
        ${LIBM}
        grass_btree2
        grass_gis
        grass_imagery
        grass_raster
        grass_vector
    SRC_REGEX "*.cpp"
)

build_program_in_subdir(
    i.biomass
    DEPENDS ${LIBM} grass_gis grass_imagery grass_raster grass_vector
)
build_program_in_subdir(
    i.cca
    DEPENDS
        ${LIBM}
        grass_gis
        grass_gmath
        grass_imagery
        grass_raster
        grass_vector
)
build_program_in_subdir(
    i.cluster
    DEPENDS grass_cluster grass_gis grass_imagery grass_raster grass_vector
)

build_program_in_subdir(
    i.eb.evapfr
    DEPENDS ${LIBM} grass_gis grass_imagery grass_raster grass_vector
)
build_program_in_subdir(
    i.eb.eta
    DEPENDS ${LIBM} grass_gis grass_imagery grass_raster grass_vector
)
build_program_in_subdir(
    i.eb.hsebal01
    DEPENDS ${LIBM} grass_gis grass_imagery grass_raster grass_vector
)
build_program_in_subdir(
    i.eb.netrad
    DEPENDS ${LIBM} grass_gis grass_imagery grass_raster grass_vector
)
build_program_in_subdir(
    i.eb.soilheatflux
    DEPENDS ${LIBM} grass_gis grass_imagery grass_raster grass_vector
)

build_program_in_subdir(
    i.evapo.mh
    DEPENDS ${LIBM} grass_gis grass_imagery grass_raster grass_vector
)
build_program_in_subdir(
    i.evapo.pm
    DEPENDS ${LIBM} grass_gis grass_imagery grass_raster grass_vector
)
build_program_in_subdir(
    i.evapo.pt
    DEPENDS ${LIBM} grass_gis grass_imagery grass_raster grass_vector
)
build_program_in_subdir(
    i.evapo.time
    DEPENDS grass_gis grass_imagery grass_raster grass_vector
)
build_program_in_subdir(
    i.emissivity
    DEPENDS ${LIBM} grass_gis grass_imagery grass_raster grass_vector
)
build_program_in_subdir(
    i.find
    DEPENDS grass_gis grass_imagery grass_raster grass_vector
    PACKAGE "grassmods"
    RUNTIME_OUTPUT_DIR ${GRASS_INSTALL_ETCDIR}
    NO_DOC
)

build_program_in_subdir(
    i.gensig
    DEPENDS ${LIBM} grass_gis grass_imagery grass_raster grass_vector
)

build_program_in_subdir(
    i.gensigset
    DEPENDS
        ${LIBM}
        grass_gis
        grass_gmath
        grass_imagery
        grass_raster
        grass_vector
)

build_program_in_subdir(
    i.group
    DEPENDS grass_gis grass_imagery grass_parson grass_raster grass_vector
)

build_program_in_subdir(
    i.his.rgb
    DEPENDS grass_gis grass_imagery grass_raster grass_vector
)

build_program_in_subdir(
    i.landsat.toar
    DEPENDS ${LIBM} grass_gis grass_imagery grass_raster grass_vector
)

build_program_in_subdir(
    i.maxlik
    DEPENDS ${LIBM} grass_gis grass_imagery grass_raster grass_vector
)

build_program_in_subdir(
    i.modis.qc
    DEPENDS grass_gis grass_imagery grass_raster grass_vector
)

build_program_in_subdir(
    i.rectify
    DEPENDS ${LIBM} grass_gis grass_imagery grass_raster grass_vector
)

build_program_in_subdir(
    i.rgb.his
    DEPENDS grass_gis grass_imagery grass_raster grass_vector
)

build_program_in_subdir(
    i.segment
    DEPENDS
        ${LIBM}
        grass_gis
        grass_imagery
        grass_raster
        grass_segment
        grass_vector
)

build_program_in_subdir(i.signatures DEPENDS grass_imagery grass_gis)

build_program_in_subdir(
    i.smap
    DEPENDS
        ${LIBM}
        grass_gis
        grass_gmath
        grass_imagery
        grass_raster
        grass_vector
)

build_program_in_subdir(
    i.target
    DEPENDS grass_gis grass_imagery grass_raster grass_vector
)

build_program_in_subdir(
    i.topo.corr
    DEPENDS ${LIBM} grass_gis grass_imagery grass_raster grass_vector
)

build_program_in_subdir(
    i.pca
    DEPENDS
        ${LIBM}
        grass_gis
        grass_gmath
        grass_imagery
        grass_raster
        grass_vector
)

build_program_in_subdir(
    i.vi
    DEPENDS ${LIBM} grass_gis grass_imagery grass_raster grass_vector
)

if(WITH_FFTW)
    build_program_in_subdir(
        i.zc
        DEPENDS grass_gis grass_gmath grass_imagery grass_raster grass_vector
    )
else()
    message(STATUS "i.zc disabled because FFTW is not available")
endif()

build_program_in_subdir(
    i.fft
    DEPENDS grass_gis grass_gmath grass_imagery grass_raster
    PRIMARY_DEPENDS FFTW
)

build_program_in_subdir(
    i.ifft
    DEPENDS grass_btree2 grass_gis grass_gmath grass_raster
    PRIMARY_DEPENDS FFTW
)

build_program_in_subdir(i.landsat.acca DEPENDS grass_raster grass_gis ${LIBM})

# TODO: add generate docs for i.ortho.photo
add_subdirectory(i.ortho.photo)
