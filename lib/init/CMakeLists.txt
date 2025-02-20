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
# set(START_UP "grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}.py")
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
get_filename_component(PROJ_INSTALL_PREFIX ${PROJ_INCLUDE_DIR} PATH)
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
#[==[
if(MSVC)
  find_path(geotiff_share_dir "epsg_csv" PATH_SUFFIXES share)
  mark_as_advanced(geotiff_share_dir)
  if(geotiff_share_dir)
    file(TO_NATIVE_PATH "${geotiff_share_dir}/epsg_csv" GEOTIFF_CSV)
  else()
    message(FATAL_ERROR "Cannot find share/epsg_csv.")
  endif()
endif() # MSVC
]==]
get_filename_component(GDAL_DIR ${GDAL_LIBRARY} PATH)
get_filename_component(GDAL_DIR ${GDAL_DIR} PATH)

file(TO_NATIVE_PATH ${GDAL_DIR}/bin DEPS_DLL_PATH)
list(APPEND DLL_PATH_LIST ${DEPS_DLL_PATH})

# file(TO_NATIVE_PATH ${PYTHON_EXECUTABLE} GRASS_PYTHON)

# For install tree (first do install tree)
set(LD_LIBRARY_PATH_VAR "LD_LIBRARY_PATH")

if(APPLE)
  set(LD_LIBRARY_PATH_VAR "LD_RUN_PATH")
elseif(WIN32)
  set(LD_LIBRARY_PATH_VAR "PATH")
endif()

#[==[ SKIP THIS
if(WIN32)
  set(app_data_dir $ENV{APPDATA})
  if(NOT EXISTS ${app_data_dir})
    # this can happen with some strange settings
    message(
      FATAL_ERROR
        "The APPDATA variable is not set, ask your operating system support")
  endif()
  file(TO_NATIVE_PATH "${app_data_dir}/GRASS8" grass_config_dir)
else()
  set(grass_config_dir "$ENV{HOME}/.grass8")
endif()

file(TO_NATIVE_PATH "${CMAKE_INSTALL_PREFIX}" gisbase_init_dir)
message(STATUS "GISBASE (install) : ${gisbase_init_dir}")
message(STATUS "grass config directory (install): ${grass_config_dir}")

# configure and install env.sh
if(NOT MSVC)
  configure_file(env.sh ${CMAKE_CURRENT_BINARY_DIR}/bashrc @ONLY)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/bashrc
          DESTINATION ${grass_config_dir})
endif()
]==]

# configure and install grass.py
set(SET_GISBASE ${RUNTIME_GISBASE})
configure_file(grass.py ${OUTDIR}/${CMAKE_INSTALL_BINDIR}/${START_UP} @ONLY)

set(SET_GISBASE ${GISBASE})
configure_file(grass.py ${CMAKE_CURRENT_BINARY_DIR}/${START_UP} @ONLY)
unset(SET_GISBASE)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${START_UP}
        DESTINATION ${CMAKE_INSTALL_BINDIR})

#[====================[ SKIP THIS
# configure and install grass wrapper script (grass.bat or grass.sh
configure_file(${script_input_file_name}
               ${CMAKE_CURRENT_BINARY_DIR}/${script_file_name} @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${script_file_name}
        DESTINATION ${CMAKE_INSTALL_BINDIR})


# For build tree
file(TO_NATIVE_PATH ${GISBASE} gisbase_init_dir)
file(TO_NATIVE_PATH "${GISBASE}/etc/config" grass_config_dir)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.20)
  configure_file(
    ${script_input_file_name}
    ${GISBASE}/${script_file_name}
    FILE_PERMISSIONS
    OWNER_READ
    OWNER_WRITE
    OWNER_EXECUTE
    GROUP_READ
    GROUP_EXECUTE
    WORLD_READ
    WORLD_EXECUTE
    @ONLY)
else()
  configure_file(${script_input_file_name} ${GISBASE}/${script_file_name} @ONLY)
  if(UNIX OR MINGW)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
      file(
        CHMOD
        ${GISBASE}/${script_file_name}
        FILE_PERMISSIONS
        OWNER_READ
        OWNER_WRITE
        OWNER_EXECUTE
        GROUP_READ
        GROUP_EXECUTE
        WORLD_READ
        WORLD_EXECUTE)
    else()
      execute_process(COMMAND /bin/chmod 755 ${GISBASE}/${script_file_name})
    endif()
  endif()
endif()
]====================]

file(TO_NATIVE_PATH "${OUTDIR}/${GISBASE_DIR}" gisbase_init_dir)

file(COPY ${CMAKE_SOURCE_DIR}/demolocation/PERMANENT
     DESTINATION ${OUTDIR}/${GRASS_INSTALL_DEMODIR})

install(DIRECTORY ${CMAKE_SOURCE_DIR}/demolocation/PERMANENT/
        DESTINATION ${GRASS_INSTALL_DEMODIR}/PERMANENT)

set(GISDBASE ${OUTDIR}/${GISBASE_DIR})
configure_file(${CMAKE_SOURCE_DIR}/demolocation/grassrc.tmpl "${GISRC}" @ONLY)
unset(GISDBASE)

file(TO_NATIVE_PATH ${OUTDIR}/${GRASS_INSTALL_BINDIR} grass_dll_dir)
set(DLL_PATH_LIST)
list(APPEND DLL_PATH_LIST ${grass_dll_dir})
separate_arguments(DLL_PATH_LIST)

set(LD_LIBRARY_PATH_VAR)
if(UNIX OR MINGW)
  if(DEFINED ENV{LD_LIBRARY_PATH})
    set(LD_LIBRARY_PATH_VAR
        "${OUTDIR}/${GRASS_INSTALL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")
  else()
    set(LD_LIBRARY_PATH_VAR "${OUTDIR}/${GRASS_INSTALL_LIBDIR}")
  endif()
endif()
message(STATUS "GISBASE (build) : ${GISBASE_DIR}")
message(STATUS "grass config directory (build): ${grass_config_dir}")
#[[
if(NOT MSVC)
  configure_file(env.sh ${grass_config_dir}/bashrc @ONLY)
  message(STATUS "env variables: ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/config/bashrc")
endif()
]]

# configure_file(grass.py ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/${START_UP} @ONLY)
# message(STATUS "Startup (build): ${GISBASE}/${script_file_name}")
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
