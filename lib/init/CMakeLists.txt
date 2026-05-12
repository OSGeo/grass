# #for i18N support

add_custom_target(ADD_INIT_DOCS ALL COMMENT "Generate init docs.")
set_target_properties(ADD_INIT_DOCS PROPERTIES FOLDER Docs)

if(WITH_DOCS)

  generate_docs_list(
    TARGET ADD_INIT_DOCS
    DOC_FILES
      grass
      helptext
      variables
    IMG_FILES
      grass_start.png
      grassdb.png
      help_project_structure.png
      location-add.png
      location-download.png
      location.png
      mapset.png
      raster-import.png
      vector-import.png)

endif()

# START_UP is the variable used in grass.py, grass.sh.in and grass.bat.in
set(START_UP ${PROJECT_NAME_LOWER})
if(WIN32)
  set(START_UP "${START_UP}.py")
  set(script_file_name "grass.bat")
  set(script_input_file_name ${script_file_name}.in)
else()
  set(script_file_name "grass")
  set(script_input_file_name ${script_file_name}.sh.in)
endif()

find_path(gdal_share_dir "gdal" PATH_SUFFIXES share)
if(NOT gdal_share_dir)
  message(FATAL_ERROR "Cannot find share/gdal")
endif()
mark_as_advanced(gdal_share_dir)

file(TO_NATIVE_PATH "${gdal_share_dir}/gdal" GDAL_DATA)
get_filename_component(GDAL_DIR "${GDAL_LIBRARY}" PATH)
get_filename_component(GDAL_DIR "${GDAL_DIR}" PATH)

file(TO_NATIVE_PATH ${GDAL_DIR}/bin DEPS_DLL_PATH)
list(APPEND DLL_PATH_LIST ${DEPS_DLL_PATH})

# configure and install grass.py
set(GRASS_PYDIR ${GRASS_INSTALL_PYDIR_RUNTIME_PATH})
configure_file(grass.py ${OUTDIR}/${CMAKE_INSTALL_BINDIR}/${START_UP} @ONLY)

set(GRASS_PYDIR ${GRASS_INSTALL_PYDIR_PATH})
configure_file(grass.py ${CMAKE_CURRENT_BINARY_DIR}/${START_UP} @ONLY)
unset(GRASS_PYDIR)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${START_UP}
        DESTINATION ${CMAKE_INSTALL_BINDIR})

file(TO_NATIVE_PATH "${OUTDIR}/${GISBASE_DIR}" gisbase_init_dir)

file(TO_NATIVE_PATH ${OUTDIR}/${GRASS_INSTALL_BINDIR} grass_dll_dir)
set(DLL_PATH_LIST)
list(APPEND DLL_PATH_LIST ${grass_dll_dir})
separate_arguments(DLL_PATH_LIST)

message(STATUS "GISBASE (build) : ${GISBASE_DIR}")
message(STATUS "grass config directory (build): ${grass_config_dir}")

message(STATUS "gisrc (build): ${GISRC}")

# finish installtree and buildtree configurations

build_program(
  NAME
  echo
  SOURCES
  echo.c
  PACKAGE
  "grasslibs"
  RUNTIME_OUTPUT_DIR
  ${GRASS_INSTALL_ETCBINDIR}
  NO_DOCS)

build_program(
  NAME
  run
  SOURCES
  run.c
  PACKAGE
  "grasslibs"
  RUNTIME_OUTPUT_DIR
  ${GRASS_INSTALL_ETCBINDIR}
  NO_DOCS)

build_program(
  NAME
  clean_temp
  SOURCES
  clean_temp.c
  DEPENDS
  grass_gis
  PACKAGE
  "grasslibs"
  RUNTIME_OUTPUT_DIR
  ${GRASS_INSTALL_ETCBINDIR}
  NO_DOCS)

build_program(
  NAME
  lock
  SOURCES
  lock.c
  DEPENDS
  grass_gis
  grass_datetime
  PACKAGE
  "grasslibs"
  RUNTIME_OUTPUT_DIR
  ${GRASS_INSTALL_ETCBINDIR}
  NO_DOCS)

if(MINGW)
  build_program(
    NAME
    winlocale
    SOURCES
    winlocale.c
    RUNTIME_OUTPUT_DIR
    ${GRASS_INSTALL_ETCBINDIR}
    NO_DOCS)
endif()

set(VERSIONNUMBER_CONTENTS "${GRASS_VERSION_NUMBER}")
if(GRASS_VERSION_GIT)
  list(APPEND VERSIONNUMBER_CONTENTS
       "${VERSIONNUMBER_CONTENTS} ${GRASS_VERSION_GIT}")
endif()

file(WRITE ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/VERSIONNUMBER
     "${VERSIONNUMBER_CONTENTS}")

install(FILES ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/VERSIONNUMBER
        DESTINATION ${GRASS_INSTALL_ETCDIR})

configure_file(license.txt "${OUTDIR}/${GRASS_INSTALL_ETCDIR}/license" @ONLY)

install(FILES ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/license
        DESTINATION ${GRASS_INSTALL_ETCDIR})
