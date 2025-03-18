#[=======================================================================[.rst:
CheckDependentLibraries.cmake
-----------------------------

Detect GRASS dependencies and set variable HAVE_*

#]=======================================================================]

# Required dependencies

find_package(FLEX REQUIRED)

find_package(BISON REQUIRED)

if(UNIX)
  find_library(MATH_LIBRARY m)
  add_library(LIBM INTERFACE IMPORTED GLOBAL)
  set_property(TARGET LIBM PROPERTY INTERFACE_LINK_LIBRARIES ${MATH_LIBRARY})
  mark_as_advanced(M_LIBRARY)
  set(LIBM LIBM)
endif()

find_package(PROJ REQUIRED)

find_package(GDAL REQUIRED)

find_package(ZLIB REQUIRED)

# Optional dependencies

if(MSVC)
  find_package(PCRE REQUIRED)
  if(PCRE_FOUND)
    add_library(PCRE INTERFACE IMPORTED GLOBAL)
    set_property(TARGET PCRE PROPERTY INTERFACE_LINK_LIBRARIES
                                      ${PCRE_LIBRARY${find_library_suffix}})
    set_property(TARGET PCRE PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                                      ${PCRE_INCLUDE_DIR})
  endif()
endif()

find_package(Iconv)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)

# Graphics options

if(WITH_X11)
  find_package(X11 REQUIRED)
endif()

if(WITH_OPENGL)
  find_package(OpenGL REQUIRED COMPONENTS OpenGL)
  if(APPLE)
    find_library(AGL_FRAMEWORK AGL REQUIRED)
    set_property(
      TARGET OpenGL::GL
      APPEND
      PROPERTY INTERFACE_LINK_LIBRARIES ${AGL_FRAMEWORK})
  endif()
endif()

if(WITH_CAIRO)
  find_package(Fontconfig REQUIRED)
  find_package(Cairo REQUIRED)
endif()

if(WITH_LIBPNG)
  find_package(PNG REQUIRED)
endif()

# Data storage options

if(WITH_SQLITE)
  find_package(SQLite3 REQUIRED)
endif()

if(WITH_POSTGRES)
  if(NOT PostgreSQL_ADDITIONAL_VERSIONS)
    set(PostgreSQL_ADDITIONAL_VERSIONS "17" "16" "15" "14" "13")
  endif()
  find_package(PostgreSQL REQUIRED)
endif()

if(WITH_MYSQL)
  find_package(MySQL REQUIRED)
  if(MySQL_FOUND)
    add_library(MYSQL INTERFACE IMPORTED GLOBAL)
    set_property(TARGET MYSQL PROPERTY INTERFACE_LINK_LIBRARIES
                                       ${MySQL_LIBRARY})
    set_property(TARGET MYSQL PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                                       ${MySQL_INCLUDE_DIRS})
  endif()
endif()

if(WITH_ODBC)
  find_package(ODBC REQUIRED)
endif()

if(WITH_ZSTD)
  find_package(zstd REQUIRED)
  if(zstd_FOUND)
    add_library(ZSTD INTERFACE IMPORTED GLOBAL)
    set_property(TARGET ZSTD PROPERTY INTERFACE_LINK_LIBRARIES
                                      ${zstd_LIBRARIES})
    set_property(TARGET ZSTD PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                                      ${zstd_INCLUDE_DIRS})
  endif()
endif()

if(WITH_BZLIB)
  find_package(BZip2 REQUIRED)
endif()

# Command-line options
if(WITH_READLINE)
  find_package(Readline REQUIRED COMPONENTS History)
endif()

# Language options
if(WITH_FREETYPE)
  find_package(Freetype REQUIRED)
endif()

if(WITH_NLS)
  find_package(Gettext REQUIRED)
  if(GETTEXT_FOUND)
    set(MSGFMT ${GETTEXT_MSGFMT_EXECUTABLE})
    set(MSGMERGE ${GETTEXT_MSGMERGE_EXECUTABLE})
  endif()
  find_package(Intl REQUIRED)
endif()

# Computing options
if(WITH_FFTW)
  find_package(FFTW REQUIRED)
  if(FFTW_FOUND)
    add_library(FFTW INTERFACE IMPORTED GLOBAL)
    set_property(TARGET FFTW PROPERTY INTERFACE_LINK_LIBRARIES
                                      ${FFTW_LIBRARIES})
    set_property(TARGET FFTW PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                                      ${FFTW_INCLUDE_DIR})
  endif()
endif()

if(WITH_CBLAS)
  if(NOT CBLAS_PREFER_PKGCONFIG)
    set(CBLAS_PREFER_PKGCONFIG ON)
  endif()
  find_package(CBLAS REQUIRED)
endif()

if(WITH_LAPACKE)
  if(NOT WITH_CBLAS)
    message(FATAL_ERROR "LAPACKE support requires CBLAS")
  endif()
  find_package(LAPACKE REQUIRED)
endif()

if(WITH_OPENMP)
  if(MSVC AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.30")
    # for min/max reduction
    # get rid of warning D9025: overriding '/openmp' with '/openmp:llvm'
    set(OpenMP_RUNTIME_MSVC "llvm")
  endif()
  find_package(OpenMP REQUIRED)
  if(OpenMP_FOUND AND MSVC AND CMAKE_VERSION VERSION_LESS "3.30")
    # CMake < 3.30 doesn't support OpenMP_RUNTIME_MSVC
    # for min/max reduction
    add_compile_options(-openmp:llvm)
  endif()
endif()

# Data format options
if(WITH_TIFF)
  find_package(TIFF REQUIRED)
endif()

if(WITH_NETCDF)
  find_package(NetCDF REQUIRED)
endif()

if(WITH_GEOS)
  find_package(GEOS REQUIRED)
  message(STATUS "Found GEOS: ${GEOS_DIR} (found version \"${GEOS_VERSION}\")")
endif()

if(WITH_PDAL)
  find_package(PDAL REQUIRED CONFIG)
  set(PDAL pdalcpp)
  if(NOT TARGET pdalcpp
     AND TARGET pdal_base
     AND TARGET pdal_util)
    # Workaround for PDAL <2.6
    set(PDAL pdal_base pdal_util)
  endif()
  message(STATUS "Found PDAL: ${PDAL_DIR} (found version \"${PDAL_VERSION}\")")
endif()

if(WITH_LIBLAS)
  find_package(LibLAS REQUIRED)
  if(LibLAS_FOUND)
    add_library(LIBLAS INTERFACE IMPORTED GLOBAL)
    set_property(TARGET LIBLAS PROPERTY INTERFACE_LINK_LIBRARIES
                                        ${LibLAS_C_LIBRARY})
    set_property(TARGET LIBLAS PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                                        ${LibLAS_INCLUDE_DIR})
  endif()
endif()

find_package(Python3 REQUIRED)
if(Python3_FOUND)
  set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
  set(PYTHON_SITEARCH ${Python3_SITEARCH})
  #[[
  find_package(PythonLibs REQUIRED)
  find_package(Numpy)
  #]]
endif()

check_target(PROJ::proj HAVE_PROJ_H)
check_target(GDAL::GDAL HAVE_GDAL)
check_target(GDAL::GDAL HAVE_OGR)
check_target(ZLIB::ZLIB HAVE_ZLIB_H)
check_target(Iconv::Iconv HAVE_ICONV_H)
check_target(PNG::PNG HAVE_PNG_H)
check_target(LIBJPEG HAVE_JPEGLIB_H)
check_target(SQLite::SQLite3 HAVE_SQLITE)
check_target(SQLite::SQLite3 HAVE_SQLITE3_H)
check_target(PostgreSQL::PostgreSQL HAVE_POSTGRES)
check_target(PostgreSQL::PostgreSQL HAVE_LIBPQ_FE_H)
check_target(MYSQL HAVE_MYSQL_H)
check_target(ODBC::ODBC HAVE_SQL_H)
check_target(ZSTD HAVE_ZSTD_H)
check_target(BZip2::BZip2 HAVE_BZLIB_H)
check_target(Readline::Readline HAVE_READLINE_READLINE_H)
check_target(Readline::History HAVE_READLINE_HISTORY_H)
check_target(Freetype::Freetype HAVE_FT2BUILD_H)
check_target(Cairo::Cairo HAVE_CAIRO_H)
# set(CMAKE_REQUIRED_INCLUDES "${FFTW_INCLUDE_DIR}") no target ATLAS in
# thirdpary/CMakeLists.txt
check_target(ATLAS HAVE_LIBATLAS)
check_target(CBLAS::CBLAS HAVE_LIBBLAS)
check_target(LAPACKE::LAPACKE HAVE_LIBLAPACK)
check_target(TIFF::TIFF HAVE_TIFFIO_H)
check_target(NETCDF HAVE_NETCDF)
check_target(GEOS::geos_c HAVE_GEOS)

if(MSVC)
  check_target(PCRE HAVE_PCRE_H)
endif()

set(HAVE_PBUFFERS 0)
set(HAVE_PIXMAPS 0)
if(WITH_OPENGL)
  try_compile(
    HAVE_PBUFFERS ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/cmake/tests/have_pbuffer.c
    CMAKE_FLAGS "-DINCLUDE_DIRECTORIES:PATH=${OPENGL_INCLUDE_DIR}" "-w"
                "-DLINK_LIBRARIES:STRING=${OPENGL_LIBRARIES}"
    OUTPUT_VARIABLE COMPILE_HAVE_PBUFFERS)
  if(NOT COMPILE_HAVE_PBUFFERS)
    message(
      FATAL_ERROR
        "Performing Test HAVE_PBUFFERS - Failed\n COMPILE_OUTPUT:${COMPILE_HAVE_PBUFFERS}\n"
    )
  else()
    message(STATUS "Performing Test HAVE_PBUFFERS - Success")
    set(HAVE_PBUFFERS 1)
  endif()

  try_compile(
    HAVE_PIXMAPS ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/cmake/tests/have_pixmaps.c
    CMAKE_FLAGS "-DINCLUDE_DIRECTORIES:PATH=${OPENGL_INCLUDE_DIR}" "-w"
                "-DLINK_LIBRARIES:STRING=${OPENGL_LIBRARIES}"
    OUTPUT_VARIABLE COMPILE_HAVE_PIXMAPS)

  if(NOT COMPILE_HAVE_PIXMAPS)
    message(
      FATAL_ERROR
        "Performing Test HAVE_PIXMAPS - Failed\n COMPILE_OUTPUT:${COMPILE_HAVE_PIXMAPS}\n"
    )
  else()
    message(STATUS "Performing Test HAVE_PIXMAPS - Success")
    set(HAVE_PIXMAPS 1)
  endif()

endif(WITH_OPENGL)

set(OPENGL_X11 0)
set(OPENGL_AQUA 0)
set(OPENGL_WINDOWS 0)
if(WITH_OPENGL)
  if(APPLE)
    set(OPENGL_AQUA 1)
    set(OPENGL_AGL 1)
  elseif(WIN32)
    set(OPENGL_WINDOWS 1)
  else()
    set(OPENGL_X11 1)
  endif()
endif()
