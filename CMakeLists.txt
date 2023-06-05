# AUTHOR(S): Rashad Kanavath <rashad km gmail>
# PURPOSE: 	 GRASS GIS root that adds options to activate/deactivate 3rd party libraries
# COPYRIGHT: (C) 2020-2022 by the GRASS Development Team
#   	    	 This program is free software under the GPL (>=v2)
#   	    	 Read the file COPYING that comes with GRASS for details.

cmake_minimum_required(VERSION 3.11)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

#if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
#  set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
#endif()

project(GRASSGIS)

set(BUILD_SHARED_LIBS ON)
#message(FATAL_ERROR "VCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}")
if(MSVC)
  if(BUILD_SHARED_LIBS)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
  endif()

  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/find_scripts;${CMAKE_SOURCE_DIR}/cmake/modules;${CMAKE_MODULE_PATH}")

set(default_option_enabled ON)
if(WIN32)
  set(default_option_enabled OFF)
endif()

# Configure CCache if available
if(NOT MSVC)
  option(USE_CCACHE "Use ccache" ON)
  if (USE_CCACHE)
    find_program(CCACHE_FOUND ccache)
    if(CCACHE_FOUND)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
        message(STATUS "ccache found")
    endif(CCACHE_FOUND)
  endif(USE_CCACHE)
endif(NOT MSVC)

if(CMAKE_BUILD_TYPE)
set( grass_build_type "${CMAKE_BUILD_TYPE}" )
string( TOLOWER "${grass_build_type}" grass_build_type_lc )
set(find_library_suffix "_RELEASE")
if( grass_build_type_lc STREQUAL "debug" )
set(find_library_suffix "_DEBUG")
endif()
else()
set(find_library_suffix "")
endif()

option(WITH_CAIRO "Build with cairo support ." ON)
option(WITH_X11 "Build with X11 support ." ${default_option_enabled})
option(WITH_OPENGL "Build with opengl support ." ON)
option(WITH_SQLITE "enable sqlite support" ON)
option(WITH_POSTGRES "enable postgres support" ON)
option(WITH_NLS "enable nls support" ${default_option_enabled})
option(WITH_BZLIB "enable bzlib support" ON)
option(WITH_BLAS "enable blas support" ON)
option(WITH_LAPACK "enable lapack support" ON)
option(WITH_LIBLAS "enable libLAS support" OFF)
option(WITH_OPENDWG "enable v.in.dwg" OFF)

option(WITH_PYTHON "Build python bindings" ON)
option(WITH_LARGEFILES "enable largefile support" ${default_option_enabled})
option(WITH_DOCS "Build documentation" ON)
if(APPLE)
  if(POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
  endif()
  set(CMAKE_MACOSX_RPATH TRUE)
endif()

set(CMAKE_SKIP_BUILD_RPATH  FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif("${isSystemDir}" STREQUAL "-1")

include(get_host_arch)
get_host_arch(BUILD_ARCH)

include(get_versions)
get_versions("include/VERSION"
  GRASS_VERSION_MAJOR
  GRASS_VERSION_MINOR
  GRASS_VERSION_RELEASE
  GRASS_VERSION_DATE)

set(GRASS_VERSION_NUMBER ${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE})
message(STATUS "GRASS_VERSION_NUMBER  =  '${GRASS_VERSION_NUMBER}'")
set(GRASS_VERSION_UPDATE_PKG "0.2")

include(set_compiler_flags)
set_compiler_flags()

include(repo_status)
repo_status("${CMAKE_CURRENT_LIST_DIR}" GRASS_VERSION_GIT)

enable_testing()

# Setup build locations.
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()

include(build_module)
include(build_program)
include(build_program_in_subdir)
include(build_library_in_subdir)
include(copy_python_files_in_subdir)
include(build_script_in_subdir)
include(build_gui_in_subdir)
include(check_target)

add_subdirectory(thirdparty)
set(MKHTML_PY ${CMAKE_BINARY_DIR}/utils/mkhtml.py)

set(GISBASE ${CMAKE_BINARY_DIR}/gisbase)
file(TO_NATIVE_PATH "${GISBASE}" GISBASE_NATIVE)
file(TO_NATIVE_PATH "${CMAKE_BINARY_DIR}/bin" BINARY_DIR)
file(TO_NATIVE_PATH "${CMAKE_BINARY_DIR}/lib" LIB_DIR)
file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}" MODULE_TOPDIR)
file(TO_NATIVE_PATH "${GISBASE}/scripts" SCRIPTS_DIR)
file(TO_NATIVE_PATH "${GISBASE}/etc/config/rc" GISRC)
file(TO_NATIVE_PATH "${GISBASE}/etc/python" ETC_PYTHON_DIR)
file(TO_NATIVE_PATH "${GISBASE}/gui/wxpython" GUI_WXPYTHON_DIR)
if(WIN32)
set(sep "\;")
set(env_path "")
else()
set(sep ":")
set(env_path ":$ENV{PATH}")
endif()


set(grass_env_command ${CMAKE_COMMAND} -E env
	"PATH=${BINARY_DIR}${sep}${SCRIPTS_DIR}${env_path}"
	"PYTHONPATH=${ETC_PYTHON_DIR}${sep}${GUI_WXPYTHON_DIR}${sep}$ENV{PYTHONPATH}"
	"LD_LIBRARY_PATH=${LIB_DIR}${sep}$ENV{LD_LIBRARY_PATH}"
	"GISBASE=${GISBASE_NATIVE}"
	"GISRC=${GISRC}"
	"LC_ALL=C"
	"LANG=C"
	"LANGUAGE=C"
	"MODULE_TOPDIR=${MODULE_TOPDIR}"
    "VERSION_NUMBER=\"${GRASS_VERSION_NUMBER}\""
    "VERSION_DATE=\"${GRASS_VERSION_DATE}\"")



set(NO_HTML_DESCR_TARGETS "g.parser;ximgview;test.raster3d.lib")
add_subdirectory(include)

include_directories("${CMAKE_BINARY_DIR}/include")
include_directories(${CMAKE_BINARY_DIR}/include/grass)
if(MSVC)
	include_directories("${CMAKE_SOURCE_DIR}/msvc")
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E echo "Creating directories in ${GISBASE}"
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/bin/
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/scripts/
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/demolocation/
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/etc/config/
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/driver/db/
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/utils/
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/lib/
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/python/
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/etc/lister/
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/etc/python/grass/lib
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/gui/wxpython/xml/
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/gui/icons/
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/gui/images/
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/docs/html/
  )

add_subdirectory(lib)
add_subdirectory(utils)
set(modules_list)


set(ALL_SUBDIRS
  general
  db
  display
  imagery
  misc
  raster
  raster3d
  scripts
  vector
  temporal
  # ps
)

foreach(d ${ALL_SUBDIRS})
  add_subdirectory(${d})
endforeach()
add_custom_target(ALL_MODULES
	COMMAND ${CMAKE_COMMAND} -E echo "Building all modules"
	DEPENDS ${modules_list})

###message(FATAL_ERROR "modules_list=${modules_list}")

if(WITH_PYTHON)
 add_subdirectory(gui)
endif()

add_subdirectory(python)

if(WITH_DOCS)
  add_subdirectory(doc)
  add_subdirectory(man)
endif() # WITH_DOCS


####add_subdirectory(locale)

# TODO: To be discussed
# add_subdirectory(testsuite)
# add_subdirectory(macosx)

if(WITH_X11)
  build_program_in_subdir(visualization/ximgview DEPENDS grass_gis X11)
endif()
