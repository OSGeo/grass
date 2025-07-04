# #for i18N support

file(GLOB init_HTMLFILES "*.html")
add_custom_target(ADD_INIT_DOCS ALL COMMENT "Generate init docs.")

if(WITH_DOCS)
  foreach(html_file ${init_HTMLFILES})
    get_filename_component(filename ${html_file} NAME_WLE)
    generate_html(TARGET ADD_INIT_DOCS NAME ${filename})
  endforeach()
endif()

# START_UP is the variable used in grass.py, grass.sh.in and grass.bat.in
set(START_UP "grass")
if(WIN32)
  set(START_UP "${START_UP}.py")
  set(script_file_name "grass.bat")
  set(script_input_file_name ${script_file_name}.in)
else()
  set(script_file_name "grass")
  set(script_input_file_name ${script_file_name}.sh.in)
endif()

file(TO_NATIVE_PATH ${CMAKE_BINARY_DIR}/bin BINARY_DIR)
set(CONFIG_PROJSHARE)
get_filename_component(PROJ_INSTALL_PREFIX ${PROJ_INCLUDE_DIRS} PATH)
if(DEFINED ENV{PROJSHARE})
  message(WARNING "External PROJ directory not specified; default will be used")
  set(CONFIG_PROJSHARE "$ENV{PROJSHARE}")
else()
  set(CONFIG_PROJSHARE "${PROJ_INSTALL_PREFIX}/share/proj")
  if(EXISTS ${CONFIG_PROJSHARE}/epsg)
    message(STATUS "PROJ directory ${CONFIG_PROJSHARE}")
  endif()
endif()
file(TO_NATIVE_PATH "${PROJ_INSTALL_PREFIX}/share/proj" PROJ_LIB)
file(TO_NATIVE_PATH ${CONFIG_PROJSHARE} GRASS_PROJSHARE)

find_path(gdal_share_dir "gdal" PATH_SUFFIXES share)
if(NOT gdal_share_dir)
  message(FATAL_ERROR "Cannot find share/gdal")
endif()
mark_as_advanced(gdal_share_dir)

file(TO_NATIVE_PATH "${gdal_share_dir}/gdal" GDAL_DATA)
get_filename_component(GDAL_DIR ${GDAL_LIBRARY} PATH)
get_filename_component(GDAL_DIR ${GDAL_DIR} PATH)

file(TO_NATIVE_PATH ${GDAL_DIR}/bin DEPS_DLL_PATH)
list(APPEND DLL_PATH_LIST ${DEPS_DLL_PATH})

# For install tree (first do install tree)
set(LD_LIBRARY_PATH_VAR "LD_LIBRARY_PATH")

if(APPLE)
  set(LD_LIBRARY_PATH_VAR "LD_RUN_PATH")
elseif(WIN32)
  set(LD_LIBRARY_PATH_VAR "PATH")
endif()

# configure and install grass.py
set(GISBASE_INSTALL_PATH ${RUNTIME_GISBASE})
configure_file(grass.py ${OUTDIR}/${CMAKE_INSTALL_BINDIR}/${START_UP} @ONLY)

set(GISBASE_INSTALL_PATH ${GISBASE})
configure_file(grass.py ${CMAKE_CURRENT_BINARY_DIR}/${START_UP} @ONLY)
unset(GISBASE_INSTALL_PATH)

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

execute_process(
  COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/version.sed "${GRASS_VERSION_NUMBER}"
          "${GRASS_VERSION_DATE}" ""
  INPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/license.txt
  OUTPUT_FILE ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/license)

install(FILES ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/license
        DESTINATION ${GRASS_INSTALL_ETCDIR})
