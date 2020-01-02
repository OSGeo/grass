
# #for i18N support
set(PACKAGE "grasslibs")

file(GLOB init_HTMLFILES "*.html")

#START_UP is the variable used in grass.py
if(WIN32)
set(script_file_name "grass.bat")
set(START_UP "grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}.py")
else()
set(script_file_name "grass.sh")
set(env_file_name "env.sh")
set(START_UP "grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}")
endif()

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
file(TO_NATIVE_PATH "${gdal_share_dir}/gdal" GDAL_DATA)

find_path(geotiff_share_dir  "epsg_csv" PATH_SUFFIXES share)
if(geotiff_share_dir)
  file(TO_NATIVE_PATH "${geotiff_share_dir}/epsg_csv" GEOTIFF_CSV)
else()
  set(GEOTIFF_CSV "")
  message(WARNING "Cannot find share/epsg_csv. GEOTIFF_CSV will not be set")
endif()

get_filename_component(GDAL_DIR ${GDAL_LIBRARY_${find_library_suffix}} PATH)
get_filename_component(GDAL_DIR ${GDAL_DIR} PATH)

file(TO_NATIVE_PATH  ${GDAL_DIR}/bin DEPS_DLL_PATH)
list(APPEND DLL_PATH_LIST ${DEPS_DLL_PATH})

file(TO_NATIVE_PATH ${PYTHON_EXECUTABLE} GRASS_PYTHON)


# For build tree
file(COPY ${CMAKE_SOURCE_DIR}/demolocation/PERMANENT DESTINATION ${CMAKE_BINARY_DIR}/demolocation/)

set(GISBASE ${CMAKE_BINARY_DIR})
file(TO_NATIVE_PATH ${CMAKE_BINARY_DIR} GISDBASE)
configure_file(${CMAKE_SOURCE_DIR}/demolocation/grassrc.tmpl 
"${CMAKE_BINARY_DIR}/etc/config/rc" @ONLY)

file(TO_NATIVE_PATH "${CMAKE_BINARY_DIR}/etc/config" GRASS_CONFIG_DIR)

file(TO_NATIVE_PATH  ${CMAKE_BINARY_DIR}/bin grass_dll_dir)
set(DLL_PATH_LIST)
list(APPEND DLL_PATH_LIST ${grass_dll_dir})

separate_arguments(DLL_PATH_LIST)

set(LD_LIBRARY_PATH_VAR)
if(UNIX OR MINGW)
  if( DEFINED ENV{LD_LIBRARY_PATH})
    set(LD_LIBRARY_PATH_VAR "$ENV{LD_LIBRARY_PATH}:${CMAKE_BINARY_DIR}/lib")
  else()
    set(LD_LIBRARY_PATH_VAR "${CMAKE_BINARY_DIR}/lib")
  endif()
endif()
message(STATUS "GISBASE (build) : ${GISBASE}")
message(STATUS "grass config directory (build): ${GRASS_CONFIG_DIR}")
if(NOT MSVC)
  configure_file(env.sh ${CMAKE_BINARY_DIR}/etc/config/bashrc  @ONLY)
  message(STATUS "env variables: ${CMAKE_BINARY_DIR}/etc/config/bashrc")
endif()
configure_file(${script_file_name}.in ${CMAKE_BINARY_DIR}/${script_file_name}  @ONLY)

configure_file(grass.py ${CMAKE_BINARY_DIR}/etc/${START_UP} @ONLY)
message(STATUS "Startup script: ${CMAKE_BINARY_DIR}/${script_file_name}")

message(STATUS "gisrc: ${CMAKE_BINARY_DIR}/etc/config/rc")

# For install tree
configure_file(${script_file_name}.in ${CMAKE_CURRENT_BINARY_DIR}/${script_file_name}  @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${script_file_name} DESTINATION bin)

if(UNIX OR MINGW)
  if( DEFINED ENV{LD_LIBRARY_PATH})
    set(LD_LIBRARY_PATH_VAR "$ENV{LD_LIBRARY_PATH}:${CMAKE_INSTALL_PREFIX}/lib")
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
	file(TO_NATIVE_PATH "${app_data_dir}/GRASS7" GRASS_CONFIG_DIR)
else()
	set(GRASS_CONFIG_DIR "$ENV{HOME}/.grass7")
endif()
set(GISBASE "${CMAKE_INSTALL_PREFIX}")

message(STATUS "GISBASE (install) : ${GISBASE}")
message(STATUS "grass config directory (install): ${GRASS_CONFIG_DIR}")

if(NOT MSVC)
  configure_file(env.sh ${CMAKE_CURRENT_BINARY_DIR}/bashrc  @ONLY)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/bashrc
  DESTINATION ${GRASS_CONFIG_DIR})
endif()

configure_file(grass.py ${CMAKE_CURRENT_BINARY_DIR}/${START_UP}  @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${START_UP} DESTINATION etc)

add_executable(echo echo.c)
install(TARGETS echo DESTINATION etc)
set_target_properties(echo
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/etc")

add_executable(run run.c)
install(TARGETS run DESTINATION etc)
set_target_properties(run
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/etc")

add_executable(clean_temp clean_temp.c)
add_dependencies(clean_temp grass_gis)
target_link_libraries(clean_temp grass_gis)
install(TARGETS clean_temp DESTINATION etc)
set_target_properties(clean_temp
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/etc")

add_executable(lock lock.c)
add_dependencies(lock grass_gis grass_datetime)
target_link_libraries(lock grass_gis grass_datetime)
set_target_properties(lock
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/etc")
install(TARGETS lock DESTINATION etc)

if(MINGW)
  add_executable(winlocale winlocale.c)
  install(TARGETS winlocale DESTINATION etc)
  set_target_properties(winlocale
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/etc")
endif()

set(VERSIONNUMBER_CONTENTS "${GRASS_VERSION_NUMBER}")
if(GRASS_VERSION_SVN)
  list(APPEND VERSIONNUMBER_CONTENTS "${VERSIONNUMBER_CONTENTS} ${GRASS_VERSION_SVN}")
endif()

file(WRITE ${CMAKE_BINARY_DIR}/VERSIONNUMBER "${VERSIONNUMBER_CONTENTS}")
install(FILES ${CMAKE_BINARY_DIR}/VERSIONNUMBER DESTINATION etc)

configure_file(license.txt.in ${CMAKE_BINARY_DIR}/license)

install(FILES ${CMAKE_BINARY_DIR}/VERSIONNUMBER DESTINATION etc)
install(FILES ${CMAKE_BINARY_DIR}/license DESTINATION etc)