#[===========================================================================[
AUTHOR(S):  Rashad Kanavath <rashad km gmail>
PURPOSE:    GRASS GIS root that adds options to activate/deactivate
            3rd party libraries
COPYRIGHT:  (C) 2020-2022 by the GRASS Development Team
            This program is free software under the GPL (>=v2)
            Read the file COPYING that comes with GRASS for details.
#]===========================================================================]

cmake_minimum_required(VERSION 3.16)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

#[[
if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
endif()
]]

project(GRASS)
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)

set(BUILD_SHARED_LIBS ON)
# message(FATAL_ERROR "VCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}")
if(MSVC)
  if(BUILD_SHARED_LIBS)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
  endif()
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

set(CMAKE_MODULE_PATH
    "${CMAKE_SOURCE_DIR}/cmake/find_scripts;${CMAKE_SOURCE_DIR}/cmake/modules;${CMAKE_MODULE_PATH}"
)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 11)
  set(CMAKE_C_STANDARD_REQUIRED ON)
endif()

set(x11_default_option_enabled ON)
set(nls_default_option_enabled ON)
if(WIN32)
  set(x11_default_option_enabled OFF)
  set(nls_default_option_enabled OFF)
elseif(APPLE)
  set(x11_default_option_enabled OFF)
endif()

# Configure CCache if available
if(NOT MSVC)
  option(USE_CCACHE "Use ccache" ON)
  if(USE_CCACHE)
    find_program(CCACHE_FOUND ccache)
    if(CCACHE_FOUND)
      set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
      set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
      message(STATUS "ccache found")
    endif(CCACHE_FOUND)
  endif(USE_CCACHE)
endif(NOT MSVC)

if(USE_ALTERNATE_LINKER)
  include("cmake/modules/linker.cmake")
endif()

if(CMAKE_BUILD_TYPE)
  set(grass_build_type "${CMAKE_BUILD_TYPE}")
  string(TOLOWER "${grass_build_type}" grass_build_type_lc)
  set(find_library_suffix "_RELEASE")
  if(grass_build_type_lc STREQUAL "debug")
    set(find_library_suffix "_DEBUG")
  endif()
else()
  set(find_library_suffix "")
endif()

# Graphics options
option(WITH_X11 "Build with X11 support ." ${x11_default_option_enabled})
option(WITH_OPENGL "Build with opengl support ." ON)
option(WITH_CAIRO "Build with cairo support ." ON)
option(WITH_LIBPNG "Build with libpng support ." OFF)

# Data storage options
option(WITH_SQLITE "Build with SQLite support" ON)
option(WITH_POSTGRES "Build with Postgres support" OFF)
option(WITH_MYSQL "Build with MySQL support" OFF)
if(WIN32)
  option(WITH_ODBC "Build with ODBC support" OFF)
endif()
option(WITH_ZSTD "Build with zstd support" ON)
option(WITH_BZLIB "Build with bzlib support" OFF)

# Command-line options
option(WITH_READLINE "Build with Readline support" OFF)

# Language options
option(WITH_FREETYPE "Build with FreeType support" ON)
option(WITH_NLS "Build with NLS support" ${nls_default_option_enabled})

# Computing options
option(WITH_FFTW "Build with FFTW support" ON)
option(WITH_BLAS "Build with BLAS support" ON)
option(WITH_LAPACK "Build with LAPACK support" ON)
option(WITH_OPENMP "Build with OpenMP support" ON)

# Data format options
option(WITH_TIFF "Build with LibTIFF support ." ON)
option(WITH_NETCDF "Build with NetCDF support" OFF)
option(WITH_GEOS "Build with GEOS support" OFF)
option(WITH_PDAL "Build with PDAL support" ON)
option(WITH_LIBLAS "Build with libLAS support" OFF)
option(WITH_OPENDWG "Build with OpenDWG support" OFF)

# Other options
option(WITH_LARGEFILES "Build with large file support"
       ${default_option_enabled})
option(WITH_DOCS "Build documentation" ON)
option(WITH_GUI "Build GUI" ON)
option(WITH_FHS "Install with FHS" OFF)

include(check_target)
include(CheckDependentLibraries)

include(get_host_arch)
get_host_arch(BUILD_ARCH)

include(get_versions)
get_versions("include/VERSION" GRASS_VERSION_MAJOR GRASS_VERSION_MINOR
             GRASS_VERSION_RELEASE GRASS_VERSION_DATE)

set(GRASS_VERSION_NUMBER
    ${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE})
message(STATUS "GRASS_VERSION_NUMBER  =  '${GRASS_VERSION_NUMBER}'")
set(GRASS_VERSION_UPDATE_PKG "0.2")

# Setup GRASS install directories
include(GRASSInstallDirs)

# Setup build locations.
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTDIR}/${GRASS_INSTALL_LIBDIR})
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTDIR}/${GRASS_INSTALL_LIBDIR})
endif()

if(APPLE)
  if(POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
  endif()
  set(CMAKE_MACOSX_RPATH TRUE)
endif()

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${GRASS_INSTALL_LIBDIR}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

#[[ Probably not needed, but leave it here in case...
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
     "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif()
]]

# TODO: a proper fix to this workaround
if(WITH_FHS)
  file(CREATE_LINK ${OUTDIR}/${GRASS_INSTALL_DEMODIR}
       ${RUNTIME_GISBASE}/demolocation SYMBOLIC)
  file(CREATE_LINK ${OUTDIR}/${GRASS_INSTALL_FONTSDIR} ${RUNTIME_GISBASE}/fonts
       SYMBOLIC)
  file(CREATE_LINK ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/colors
       ${RUNTIME_GISBASE}/etc/colors SYMBOLIC)
  file(CREATE_LINK ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/colors.desc
       ${RUNTIME_GISBASE}/etc/colors.desc SYMBOLIC)
  file(CREATE_LINK ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/element_list
       ${RUNTIME_GISBASE}/etc/element_list SYMBOLIC)
  file(CREATE_LINK ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/renamed_options
       ${RUNTIME_GISBASE}/etc/renamed_options SYMBOLIC)
  file(CREATE_LINK ${OUTDIR}/${GRASS_INSTALL_ETCDIR}/VERSIONNUMBER
       ${RUNTIME_GISBASE}/etc/VERSIONNUMBER SYMBOLIC)
  file(MAKE_DIRECTORY "${GISBASE}/gui/wxpython")
  file(CREATE_LINK ${OUTDIR}/${GRASS_INSTALL_GUIDIR}/wxpython/xml
       ${RUNTIME_GISBASE}/gui/wxpython/xml SYMBOLIC)
endif()

include(set_compiler_flags)
set_compiler_flags()
include(Configure)

include(repo_status)
repo_status("${CMAKE_CURRENT_LIST_DIR}")

enable_testing()

include(build_module)
include(build_program)
include(build_program_in_subdir)
include(build_library_in_subdir)
include(copy_python_files_in_subdir)
include(build_script_in_subdir)
include(build_gui_in_subdir)
include(generate_html)

set(MKHTML_PY ${CMAKE_BINARY_DIR}/utils/mkhtml.py)
set(THUMBNAILS_PY ${CMAKE_BINARY_DIR}/utils/thumbnails.py)
set(HTML2MAN VERSION_NUMBER=${GRASS_VERSION_NUMBER} ${PYTHON_EXECUTABLE}
             ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}/g.html2man.py)

if(WIN32)
  set(sep "\;")
else()
  set(sep ":")
endif()

set(env_path "$ENV{PATH}")

set(grass_env_command
    ${CMAKE_COMMAND} -E env "PATH=${BIN_DIR}${sep}${SCRIPTS_DIR}${sep}${env_path}"
    "PYTHONPATH=${ETC_PYTHON_DIR}${sep}${GUI_WXPYTHON_DIR}${sep}$ENV{PYTHONPATH}"
    "LD_LIBRARY_PATH=${LIB_DIR}${sep}$ENV{LD_LIBRARY_PATH}"
    "GISBASE=${RUN_GISBASE_NATIVE}" "GISRC=${GISRC}" "LC_ALL=C" "LANG=C"
    "LANGUAGE=C" "MODULE_TOPDIR=${MODULE_TOPDIR}" "HTMLDIR=${DOC_DIR}"
    "VERSION_NUMBER=\"${GRASS_VERSION_NUMBER}\""
    "VERSION_DATE=\"${GRASS_VERSION_DATE}\"")

set(NO_HTML_DESCR_TARGETS "g.parser;ximgview;test.raster3d.lib")

add_subdirectory(include)

include_directories("${OUTDIR}/${GRASS_INSTALL_INCLUDEDIR}")
if(MSVC)
  include_directories("${CMAKE_SOURCE_DIR}/msvc")
endif()

# TODO: fix this hack
include_directories(${PostgreSQL_INCLUDE_DIR})
include_directories(${PROJ_INCLUDE_DIR})
include_directories(/opt/local/include)

add_subdirectory(lib)
add_subdirectory(python)
add_subdirectory(utils)
set(modules_list)

set(ALL_SUBDIRS
    db
    display
    general
    imagery
    misc
    ps
    raster
    raster3d
    scripts
    temporal
    vector)

foreach(d ${ALL_SUBDIRS})
  add_subdirectory(${d})
endforeach()
add_custom_target(
  ALL_MODULES
  DEPENDS ${modules_list}
  COMMENT "Building all modules.")

if(WITH_GUI)
  add_subdirectory(gui)
endif()

if(WITH_DOCS)
  add_subdirectory(doc)
  add_subdirectory(man)
endif()

add_custom_target(
  r_colors_thumbnails ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory
          ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/colortables
  COMMAND ${grass_env_command} ${PYTHON_EXECUTABLE} ${THUMBNAILS_PY}
  BYPRODUCTS ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/colortables
  COMMENT "Creating thumbnails"
  DEPENDS ALL_MODULES)
install(DIRECTORY ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/colortables
        DESTINATION ${GRASS_INSTALL_DOCDIR})

set(misc_files
    AUTHORS
    CHANGES
    CITING
    contributors_extra.csv
    contributors.csv
    COPYING
    GPL.TXT
    INSTALL.md
    REQUIREMENTS.md
    translators.csv)
install(FILES ${misc_files} DESTINATION ${GRASS_INSTALL_MISCDIR})

if(WITH_NLS)
  add_subdirectory(locale)
endif()

# TODO: To be discussed - add_subdirectory(testsuite) - add_subdirectory(macosx)

if(WITH_X11)
  build_program_in_subdir(visualization/ximgview DEPENDS grass_gis X11)
endif()
