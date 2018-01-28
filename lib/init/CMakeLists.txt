# MODULE_TOPDIR = ../..

# include $(MODULE_TOPDIR)/include/Make/Other.make
# include $(MODULE_TOPDIR)/include/Make/Compile.make

# #for i18N support
set(PACKAGE "grasslibs")

set(START_UP grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR} )

file(GLOB init_HTMLFILES "*.html")

set(BINDIR "${CMAKE_BINARY_DIR}/bin")
configure_file(grass.sh ${CMAKE_BINARY_DIR}/bin/grass.sh  @ONLY)

set(BINDIR "${CMAKE_INSTALL_PREFIX}/bin")
configure_file(grass.sh ${CMAKE_CURRENT_BINARY_DIR}/grass.sh  @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/grass.sh DESTINATION bin)

set(GRASS_CONFIG_DIR)

set(PROJSHARE)

if(DEFINED ENV{PROJSHARE})
  message(WARNING "External PROJ.4 directory not specified; default will be used")
  set(PROJSHARE "$ENV{PROJSHARE}")
else()
  get_filename_component(PROJ_INSTALL_PREFIX ${PROJ4_LIBRARY} PATH)
  set(PROJSHARE "${PROJ_INSTALL_PREFIX}/share/proj")
  if(EXISTS ${PROJSHARE}/epsg)
    message("PROJ.4 directory ${PROJSHARE}")
  endif()
endif()
set(CONFIG_PROJSHARE "${PROJSHARE}")


set(LD_LIBRARY_PATH_VAR)
if(UNIX OR MINGW)
  if( DEFINED ENV{LD_LIBRARY_PATH})
    set(LD_LIBRARY_PATH_VAR "$ENV{LD_LIBRARY_PATH}:${CMAKE_BINARY_DIR}/lib")
  else()
    set(LD_LIBRARY_PATH_VAR "${CMAKE_BINARY_DIR}/lib")
  endif()
endif()

configure_file(grass.py ${CMAKE_BINARY_DIR}/${START_UP}.tmp @ONLY)
set(GISBASE "${RUN_GISBASE}")
configure_file(grass.py ${CMAKE_BINARY_DIR}/${START_UP}.py @ONLY)


#rest here
set(GISBASE "" ) #${RUN_GISBASE}")
if(UNIX OR MINGW)
  if( DEFINED ENV{LD_LIBRARY_PATH})
    set(LD_LIBRARY_PATH_VAR "$ENV{LD_LIBRARY_PATH}:${CMAKE_INSTALL_PREFIX}/lib")
  else()
    set(LD_LIBRARY_PATH_VAR "${CMAKE_INSTALL_PREFIX}/lib")
  endif()
endif()

set(OUTPUT_STARTUP_FILE "${START_UP}")
if(MINGW)
  set(OUTPUT_STARTUP_FILE "${START_UP}.py")
endif()  
configure_file(grass.py ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_STARTUP_FILE}  @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_STARTUP_FILE} DESTINATION bin)


add_executable(echo echo.c)
install(TARGETS echo DESTINATION etc)

add_executable(run run.c)
install(TARGETS run DESTINATION etc)
    
add_executable(clean_temp clean_temp.c)
add_dependencies(clean_temp grass_gis)
target_link_libraries(clean_temp grass_gis)
install(TARGETS clean_temp DESTINATION etc)

add_executable(lock lock.c)
add_dependencies(lock grass_gis grass_datetime)
target_link_libraries(lock grass_gis grass_datetime)
install(TARGETS lock DESTINATION etc)

if(MINGW)
  add_executable(winlocale winlocale.c)
  install(TARGETS winlocale DESTINATION etc)
endif()

set(VERSIONNUMBER_CONTENTS "${GRASS_VERSION_NUMBER}")
if(GRASS_VERSION_SVN)
  list(APPEND VERSIONNUMBER_CONTENTS "${VERSIONNUMBER_CONTENTS} ${GRASS_VERSION_SVN}")
endif()

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/VERSIONNUMBER "${VERSIONNUMBER_CONTENTS}")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/VERSIONNUMBER DESTINATION etc)

configure_file(license.txt.in ${CMAKE_CURRENT_BINARY_DIR}/license)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/VERSIONNUMBER DESTINATION etc)
