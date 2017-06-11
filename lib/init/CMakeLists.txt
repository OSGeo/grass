
# #for i18N support

file(GLOB init_HTMLFILES "*.html")

#START_UP is the variable used in grass.py, grass.sh.in and grass.bat.in
set(START_UP "grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}.py")
if(WIN32)
set(script_file_name "grass.bat")
else()
set(script_file_name "grass.sh")
endif()

file(TO_NATIVE_PATH  ${CMAKE_BINARY_DIR}/bin BINARY_DIR)

set(CONFIG_PROJSHARE)
get_filename_component(PROJ_INSTALL_PREFIX ${PROJ_INCLUDE_DIR} PATH)
if(DEFINED ENV{PROJSHARE})
  message(WARNING "External PROJ.4 directory not specified; default will be used")
  set(CONFIG_PROJSHARE "$ENV{PROJSHARE}")
else()
  set(CONFIG_PROJSHARE "${PROJ_INSTALL_PREFIX}/share/proj")
  if(EXISTS ${CONFIG_PROJSHARE}/epsg)
    message(STATUS "PROJ.4 directory ${CONFIG_PROJSHARE}")
  endif()
endif()
file(TO_NATIVE_PATH "${PROJ_INSTALL_PREFIX}/share/proj" PROJ_LIB)
file(TO_NATIVE_PATH ${CONFIG_PROJSHARE} GRASS_PROJSHARE)

find_path(gdal_share_dir  "gdal" PATH_SUFFIXES share)
if(NOT gdal_share_dir)
  message(FATAL_ERROR "Cannot find share/gdal")
endif()
mark_as_advanced(gdal_share_dir)

file(TO_NATIVE_PATH "${gdal_share_dir}/gdal" GDAL_DATA)

if(MSVC)
  find_path(geotiff_share_dir  "epsg_csv" PATH_SUFFIXES share)
  mark_as_advanced(geotiff_share_dir)
  if(geotiff_share_dir)
    file(TO_NATIVE_PATH "${geotiff_share_dir}/epsg_csv" GEOTIFF_CSV)
  else()
    message(FATAL_ERROR "Cannot find share/epsg_csv.")
  endif()
endif() #MSVC

get_filename_component(GDAL_DIR ${GDAL_LIBRARY} PATH)
get_filename_component(GDAL_DIR ${GDAL_DIR} PATH)

file(TO_NATIVE_PATH  ${GDAL_DIR}/bin DEPS_DLL_PATH)
list(APPEND DLL_PATH_LIST ${DEPS_DLL_PATH})

file(TO_NATIVE_PATH ${PYTHON_EXECUTABLE} GRASS_PYTHON)

# For install tree (first do install tree)
if(UNIX OR MINGW)
  if( DEFINED ENV{LD_LIBRARY_PATH})
    set(LD_LIBRARY_PATH_VAR "${CMAKE_INSTALL_PREFIX}/lib:$ENV{LD_LIBRARY_PATH}")
  else()
    set(LD_LIBRARY_PATH_VAR "${CMAKE_INSTALL_PREFIX}/lib")
  endif()
endif()

if(WIN32)
  set(app_data_dir $ENV{APPDATA})
  if(NOT EXISTS ${app_data_dir})
    # this can happen with some strange settings
    message(FATAL_ERROR "The APPDATA variable is not set, ask your operating system support")
  endif()
  file(TO_NATIVE_PATH "${app_data_dir}/GRASS7" grass_config_dir)
else()
  set(grass_config_dir "$ENV{HOME}/.grass7")
endif()

file(TO_NATIVE_PATH "${CMAKE_INSTALL_PREFIX}" gisbase_init_dir)
message(STATUS "GISBASE (install) : ${gisbase_init_dir}")
message(STATUS "grass config directory (install): ${grass_config_dir}")

#configure and install env.sh
if(NOT MSVC)
  configure_file(env.sh ${CMAKE_CURRENT_BINARY_DIR}/bashrc  @ONLY)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/bashrc
  DESTINATION ${grass_config_dir})
endif()

#configure and install grass.py
configure_file(grass.py ${CMAKE_CURRENT_BINARY_DIR}/etc/${START_UP} @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/etc/${START_UP} DESTINATION etc)

#configure and install grass wrapper script (grass.bat or grass.sh
configure_file(${script_file_name}.in ${CMAKE_CURRENT_BINARY_DIR}/${script_file_name}  @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${script_file_name} DESTINATION bin)


# For build tree
file(TO_NATIVE_PATH ${GISBASE} gisbase_init_dir)
file(TO_NATIVE_PATH "${GISBASE}/etc/config" grass_config_dir)
configure_file(${script_file_name}.in ${GISBASE}/${script_file_name}  @ONLY)
file(COPY ${CMAKE_SOURCE_DIR}/demolocation/PERMANENT DESTINATION ${GISBASE}/demolocation/)

configure_file(${CMAKE_SOURCE_DIR}/demolocation/grassrc.cmake.in
"${grass_config_dir}/rc" @ONLY)

file(TO_NATIVE_PATH  ${GISBASE}/bin grass_dll_dir)
set(DLL_PATH_LIST)
list(APPEND DLL_PATH_LIST ${grass_dll_dir})
separate_arguments(DLL_PATH_LIST)

set(LD_LIBRARY_PATH_VAR)
if(UNIX OR MINGW)
  if( DEFINED ENV{LD_LIBRARY_PATH})
    set(LD_LIBRARY_PATH_VAR "${GISBASE}/lib:$ENV{LD_LIBRARY_PATH}")
  else()
    set(LD_LIBRARY_PATH_VAR "${GISBASE}/lib")
  endif()
endif()
message(STATUS "GISBASE (build) : ${gisbase_init_dir}")
message(STATUS "grass config directory (build): ${grass_config_dir}")
if(NOT MSVC)
  configure_file(env.sh ${grass_config_dir}/bashrc  @ONLY)
  message(STATUS "env variables: ${GISBASE}/etc/config/bashrc")
endif()

configure_file(grass.py ${GISBASE}/etc/${START_UP} @ONLY)
message(STATUS "Startup (build): ${GISBASE}/${script_file_name}")
message(STATUS "gisrc (build): ${GISBASE}/etc/config/rc")

#finish installtree and buildtree configurations


build_program(NAME echo
  SOURCES echo.c
  PACKAGE "grasslibs"
  RUNTIME_OUTPUT_DIR etc)

build_program(NAME run
  SOURCES run.c
  PACKAGE "grasslibs"
  RUNTIME_OUTPUT_DIR etc)

build_program(NAME clean_temp
  SOURCES clean_temp.c
  DEPENDS grass_gis
  PACKAGE "grasslibs"
  RUNTIME_OUTPUT_DIR etc)

build_program(NAME lock
  SOURCES lock.c
  DEPENDS grass_gis grass_datetime
  PACKAGE "grasslibs"
  RUNTIME_OUTPUT_DIR etc)

if(MINGW)
build_program(NAME winlocale
  SOURCES winlocale.c
  RUNTIME_OUTPUT_DIR etc)
endif()

set(VERSIONNUMBER_CONTENTS "${GRASS_VERSION_NUMBER}")
if(GRASS_VERSION_GIT)
  list(APPEND VERSIONNUMBER_CONTENTS "${VERSIONNUMBER_CONTENTS} ${GRASS_VERSION_GIT}")
endif()

file(WRITE ${CMAKE_BINARY_DIR}/gisbase/etc/VERSIONNUMBER "${VERSIONNUMBER_CONTENTS}")
install(FILES ${CMAKE_BINARY_DIR}/gisbase/etc/VERSIONNUMBER DESTINATION etc)

configure_file(license.txt.in ${CMAKE_BINARY_DIR}/gisbase/etc/license)

install(FILES ${CMAKE_BINARY_DIR}/gisbase/etc/VERSIONNUMBER DESTINATION etc)
install(FILES ${CMAKE_BINARY_DIR}/gisbase/etc/license DESTINATION etc)
