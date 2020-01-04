cmake_minimum_required(VERSION 3.3)

project(GRASSGIS)

set(BUILD_SHARED_LIBS ON)
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
set( grass_build_type "${CMAKE_BUILD_TYPE}" )
string( TOLOWER "${grass_build_type}" grass_build_type_lc )
set(find_library_suffix "RELEASE")
if( grass_build_type_lc STREQUAL "debug" )
set(find_library_suffix "DEBUG")
endif()

option(WITH_CAIRO "Build with cairo support ." ON)
option(WITH_X11 "Build with X11 support ." ${default_option_enabled})
option(WITH_OPENGL "Build with opengl support ." ON)
option(WITH_SQLITE "enable sqlite support" ON)
option(WITH_POSTGRES "enable postgres support" OFF)
option(WITH_NLS "enable nls support" ${default_option_enabled})
option(WITH_BZLIB "enable bzlib support" ON)
option(WITH_BLAS "enable blas support" ON)
option(WITH_LAPACK "enable lapack support" ON)
option(WITH_PYTHON "Build python bindings" OFF)
option(WITH_LARGEFILES "enable largefile support" ${default_option_enabled})
option(WITH_DOCS "Build documentation" OFF)
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
set(VERSION_DATE 2019)
file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR} module_top_dir)
file(TO_NATIVE_PATH ${CMAKE_BINARY_DIR} bin_dir)
configure_file(cmake/run_python.bat ${CMAKE_BINARY_DIR}/tools/run_python.bat @ONLY)
configure_file(cmake/run_grass.bat ${CMAKE_BINARY_DIR}/tools/run_grass.bat @ONLY)

include(set_compiler_flags)
set_compiler_flags()

include(repo_status)
repo_status("${CMAKE_CURRENT_LIST_DIR}" GRASS_VERSION_GIT)

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

include(update_per_group_target)
include(build_module)
include(build_program)
include(build_program_in_subdir)
include(build_library_in_subdir)
include(build_grass_script)
include(build_script_in_subdir)
include(check_target)

add_subdirectory(thirdparty)

add_subdirectory(include)

if(MSVC)
  include_directories("${CMAKE_SOURCE_DIR}/msvc/")
endif()

include_directories("${CMAKE_BINARY_DIR}/include")

add_subdirectory(lib)

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
  tools
  # ps
  # locale
)

foreach(d ${ALL_SUBDIRS})
  add_subdirectory(${d})
endforeach()

if(WITH_PYTHON)
  add_subdirectory(gui)
endif()

if(WITH_DOCS)
  add_subdirectory(doc)
  if(NOT MSVC)
   add_subdirectory(man)
  endif()
endif() # WITH_DOCS

# TODO: To be discussed
# add_subdirectory(testsuite)
# add_subdirectory(macosx)

if(WITH_X11)
  build_program_in_subdir(visualization/ximgview DEPENDS grass_gis X11)
endif()
