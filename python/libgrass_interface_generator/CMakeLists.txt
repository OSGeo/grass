#[[
COPYRIGHT: (c) 2020-2025 by the GRASS Development Team
SPDX-License-Identifier: GPL-2.0-or-later
]]

set(MODULES
    date
    gis
    raster
    gmath
    proj
    imagery
    vector
    rtree
    display
    stats
    dbmi
    raster3d
    arraystats
    cluster
    vedit
    segment
    rowio
    temporal)

if(TARGET grass_ogsf)
  list(APPEND MODULES ogsf nviz)
endif()

set(date_HDRS datetime.h defs/datetime.h)
set(date_LIBS grass_datetime)
set(date_TARGET_NAME grass_datetime_py)

set(gis_HDRS gis.h defs/gis.h colors.h defs/colors.h)
set(raster_HDRS raster.h defs/raster.h)
set(gmath_HDRS gmath.h defs/gmath.h)
set(proj_HDRS gprojects.h defs/gprojects.h)
set(proj_LIBS grass_gproj)
set(vector_HDRS vector.h defs/vector.h vect/dig_structs.h vect/dig_defines.h
                vect/dig_externs.h)
set(display_HDRS display.h defs/display.h)
set(dbmi_HDRS dbmi.h defs/dbmi.h)
set(dbmi_LIBS grass_dbmibase grass_dbmiclient)
set(arraystats_HDRS arraystats.h defs/arraystats.h)
set(vedit_HDRS vedit.h defs/vedit.h)
set(nviz_HDRS nviz.h defs/nviz.h)
set(rowio_HDRS rowio.h defs/rowio.h)
set(temporal_HDRS temporal.h)
set(ogsf_HDRS ogsf.h defs/ogsf.h)
set(segment_HDRS segment.h defs/segment.h)
set(imagery_HDRS imagery.h defs/imagery.h)
set(rtree_HDRS rtree.h)
set(stats_HDRS stats.h defs/stats.h)
set(raster3d_HDRS raster3d.h defs/raster3d.h)
set(cluster_HDRS cluster.h defs/cluster.h)

set(proj_INCHDRS ${PROJ_INCLUDE_DIRS} ${GDAL_INCLUDE_DIRS})
set(vector_INCHDRS ${PostgreSQL_INCLUDE_DIRS} ${GDAL_INCLUDE_DIRS})
set(vedit_INCHDRS ${PostgreSQL_INCLUDE_DIRS} ${GDAL_INCLUDE_DIRS})

set(_c_flags ${CMAKE_C_FLAGS})
if(APPLE AND CMAKE_OSX_SYSROOT)
  string(APPEND _c_flags " --sysroot ${CMAKE_OSX_SYSROOT}")
  message("_c_flags")
endif()

foreach(module ${MODULES})
  if(NOT ${module}_LIBS)
    set(${module}_LIBS grass_${module})
  endif()

  if(NOT ${module}_HDRS)
    message(FATAL_ERROR "${module}_HDRS is not set")
  endif()

  if(NOT ${module}_INCHDRS)
    set(${module}_INCHDRS})
  endif()
  list(INSERT ${module}_INCHDRS 0 ${OUTDIR}/${GRASS_INSTALL_INCLUDEDIR})
  if(CMAKE_INSTALL_INCLUDEDIR)
    list(INSERT ${module}_INCHDRS 0
         ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
  endif()
  if(CMAKE_PREFIX_PATH)
    list(INSERT ${module}_INCHDRS 0 ${CMAKE_PREFIX_PATH})
  endif()
  list(REMOVE_DUPLICATES ${module}_INCHDRS)

  if(NOT ${module}_TARGET_NAME)
    list(GET ${module}_LIBS 0 first_lib)
    set(${module}_TARGET_NAME ${first_lib}_py)
  endif()

  if(NOT ${module}_TARGET_NAME)
    message(FATAL_ERROR "${${module}_TARGET_NAME} is not set")
  endif()

  set(output_file "${OUTDIR}/${GRASS_INSTALL_PYDIR}/grass/lib/${module}.py")
  add_custom_command(
    OUTPUT ${output_file}
    DEPENDS ${${module}_LIBS}
    COMMAND
      ${CMAKE_COMMAND} -DCTYPESGEN_PY=${CMAKE_CURRENT_SOURCE_DIR}/run.py
      -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE} -DCOMPILER=${CMAKE_C_COMPILER}
      "-DHDRS=${${module}_HDRS}" "-DLIBS=${${module}_LIBS}" -DOUTDIR=${OUTDIR}
      "-DINCHDRS=${${module}_INCHDRS}" -DC_FLAGS=${_c_flags}
      -DGRASS_INSTALL_INCLUDEDIR=${GRASS_INSTALL_INCLUDEDIR}
      -DGRASS_INSTALL_SCRIPTDIR=${GRASS_INSTALL_SCRIPTDIR}
      -DGRASS_INSTALL_DEMODIR=${GRASS_INSTALL_DEMODIR}
      -DGRASS_INSTALL_LIBDIR=${GRASS_INSTALL_LIBDIR}
      -DRUNTIME_GISBASE=${RUNTIME_GISBASE} -DOUT_FILE=${output_file}
      -DGRASS_VERSION_NUMBER=${GRASS_VERSION_NUMBER}
      -DGRASS_VERSION_MAJOR=${GRASS_VERSION_MAJOR}
      -DGRASS_VERSION_MINOR=${GRASS_VERSION_MINOR} -P
      ${CMAKE_SOURCE_DIR}/cmake/ctypesgen.cmake
    COMMENT "Generating ${output_file}"
    VERBATIM)

  add_custom_target(${${module}_TARGET_NAME} ALL DEPENDS ${output_file})
  set_target_properties(${${module}_TARGET_NAME} PROPERTIES FOLDER lib/python)
  install(FILES ${output_file} DESTINATION ${GRASS_INSTALL_PYDIR}/grass/lib)

endforeach()

file(COPY_FILE libgrass__init__.py
     ${OUTDIR}/${GRASS_INSTALL_PYDIR}/grass/lib/__init__.py)
install(FILES ${OUTDIR}/${GRASS_INSTALL_PYDIR}/grass/lib/__init__.py
        DESTINATION ${GRASS_INSTALL_PYDIR}/grass/lib)

unset(_c_flags)
