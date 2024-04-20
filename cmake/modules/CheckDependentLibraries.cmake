#[=======================================================================[.rst:
CheckDependentLibraries.cmake
-----------------------------

Detect GRASS dependencies and set variable HAVE_*

#]=======================================================================]

find_package(FLEX REQUIRED)

find_package(BISON REQUIRED)

find_package(PROJ REQUIRED)
if(PROJ_FOUND)
  add_library(PROJ INTERFACE IMPORTED GLOBAL)
  set_property(TARGET PROJ PROPERTY INTERFACE_LINK_LIBRARIES
                                    ${PROJ_LIBRARY${find_library_suffix}})
  set_property(TARGET PROJ PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                                    ${PROJ_INCLUDE_DIR})
endif()

find_package(GDAL REQUIRED)
if(GDAL_FOUND)
  add_library(GDAL INTERFACE IMPORTED GLOBAL)
  set_property(TARGET GDAL PROPERTY INTERFACE_LINK_LIBRARIES ${GDAL_LIBRARY})
  set_property(TARGET GDAL PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                                    ${GDAL_INCLUDE_DIR})
endif()

find_package(PNG REQUIRED)
if(PNG_FOUND)
  add_library(LIBPNG INTERFACE IMPORTED GLOBAL)
  set_property(TARGET LIBPNG PROPERTY INTERFACE_LINK_LIBRARIES
                                      ${PNG_LIBRARY${find_library_suffix}})
  set_property(TARGET LIBPNG PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                                      ${PNG_INCLUDE_DIR})
endif()

find_package(ZLIB REQUIRED)
if(ZLIB_FOUND)
  add_library(ZLIB INTERFACE IMPORTED GLOBAL)
  set_property(TARGET ZLIB PROPERTY INTERFACE_LINK_LIBRARIES
                                    ${ZLIB_LIBRARY${find_library_suffix}})
  set_property(TARGET ZLIB PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                                    ${ZLIB_INCLUDE_DIR})
endif()

if(UNIX)
  find_library(M_LIBRARY m)
  add_library(LIBM INTERFACE IMPORTED GLOBAL)
  set_property(TARGET LIBM PROPERTY INTERFACE_LINK_LIBRARIES ${M_LIBRARY})
  mark_as_advanced(M_LIBRARY)
endif()

find_package(Freetype REQUIRED)
if(FREETYPE_FOUND)
  add_library(FREETYPE INTERFACE IMPORTED GLOBAL)
  set_property(
    TARGET FREETYPE PROPERTY INTERFACE_LINK_LIBRARIES
                             ${FREETYPE_LIBRARY${find_library_suffix}})
  set_property(TARGET FREETYPE PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                                        ${FREETYPE_INCLUDE_DIRS})
endif()

find_package(FFTW REQUIRED)
if(FFTW_FOUND)
  add_library(FFTW INTERFACE IMPORTED GLOBAL)
  set_property(TARGET FFTW PROPERTY INTERFACE_LINK_LIBRARIES ${FFTW_LIBRARIES})
  set_property(TARGET FFTW PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                                    ${FFTW_INCLUDE_DIR})
endif()

if(WITH_CAIRO)
  find_package(Cairo REQUIRED)
  if(CAIRO_FOUND)
    add_library(CAIRO INTERFACE IMPORTED GLOBAL)
    set_property(TARGET CAIRO PROPERTY INTERFACE_LINK_LIBRARIES
                                       ${CAIRO_LIBRARIES})
    set_property(TARGET CAIRO PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                                       ${CAIRO_INCLUDE_DIRS})
  endif()
endif()

if(WITH_X11)
  find_package(X11 REQUIRED)
  if(X11_FOUND)
    add_library(X11 INTERFACE IMPORTED GLOBAL)
    set_property(TARGET X11 PROPERTY INTERFACE_LINK_LIBRARIES ${X11_LIBRARIES})
    set_property(TARGET X11 PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                                     ${X11_INCLUDE_DIR})
  endif()
endif()

if(WIN32)
  find_package(ODBC QUIET)
  if(ODBC_FOUND)
    add_library(ODBC INTERFACE IMPORTED GLOBAL)
    #[[
    set_property(TARGET ODBC PROPERTY INTERFACE_LINK_LIBRARIES ${ODBC_LIBRARIES})
    set_property(TARGET PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${ODBC_INCLUDE_DIRS})
    #]]
  endif()
endif()

find_package(TIFF REQUIRED)
if(TIFF_FOUND)
  add_library(TIFF INTERFACE IMPORTED GLOBAL)
  set_property(TARGET TIFF PROPERTY INTERFACE_LINK_LIBRARIES
                                    ${TIFF_LIBRARY${find_library_suffix}})
  set_property(TARGET TIFF PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                                    ${TIFF_INCLUDE_DIR})
endif()

find_package(Iconv QUIET)
if(ICONV_FOUND)
  add_library(ICONV INTERFACE IMPORTED GLOBAL)
  set_property(TARGET ICONV PROPERTY INTERFACE_LINK_LIBRARIES
                                     ${ICONV_LIBRARIES})
  set_property(TARGET ICONV PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                                     ${ICONV_INCLUDE_DIR})
  # if(ICONV_SECOND_ARGUMENT_IS_CONST) set() update this value in
  # include/config.cmake.in
endif()

if(WITH_BZLIB)
  find_package(BZip2)
  if(BZIP2_FOUND)
    add_library(BZIP2 INTERFACE IMPORTED GLOBAL)
    set_property(TARGET BZIP2 PROPERTY INTERFACE_LINK_LIBRARIES
                                       ${BZIP2_LIBRARY${find_library_suffix}})
    set_property(TARGET BZIP2 PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                                       ${BZIP2_INCLUDE_DIR})
  endif()
endif()

if(WITH_BLAS)
  find_package(BLAS)
  if(BLAS_FOUND)
    add_library(BLAS INTERFACE IMPORTED GLOBAL)
    set_property(TARGET BLAS PROPERTY INTERFACE_LINK_LIBRARIES
                                      ${BLAS_LIBRARIES})
  endif()
endif()

if(WITH_LAPACK)
  find_package(LAPACK)
  if(LAPACK_FOUND)
    add_library(LAPACK INTERFACE IMPORTED GLOBAL)
    set_property(TARGET LAPACK PROPERTY INTERFACE_LINK_LIBRARIES
                                        ${LAPACK_LIBRARIES})
  endif()
endif()

if(WITH_OPENGL)
  find_package(OpenGL REQUIRED)
  if(OPENGL_FOUND)
    add_library(OPENGL INTERFACE IMPORTED GLOBAL)
    if(APPLE)
      find_library(AGL_FRAMEWORK AGL DOC "AGL lib for OSX")
      set(APP "-framework AGL -framework ApplicationServices")
    endif()
    set_property(TARGET OPENGL PROPERTY INTERFACE_LINK_LIBRARIES
                                        ${OPENGL_LIBRARIES} ${AGL_FRAMEWORK})
    set_property(TARGET OPENGL PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                                        ${OPENGL_INCLUDE_DIR} ${AGL_FRAMEWORK})
  endif()
endif()

if(WITH_SQLITE)
  find_package(SQLite REQUIRED)
  if(SQLITE_FOUND)
    add_library(SQLITE INTERFACE IMPORTED GLOBAL)
    set_property(TARGET SQLITE PROPERTY INTERFACE_LINK_LIBRARIES
                                        ${SQLITE_LIBRARY})
    set_property(TARGET SQLITE PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                                        ${SQLITE_INCLUDE_DIRS})
  endif()
endif()

if(WITH_POSTGRES)
  find_package(PostgreSQL)
  if(PostgreSQL_FOUND)
    add_library(POSTGRES INTERFACE IMPORTED GLOBAL)
    set_property(TARGET POSTGRES PROPERTY INTERFACE_LINK_LIBRARIES
                                          ${PostgreSQL_LIBRARY})
    set_property(TARGET POSTGRES PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                                          ${PostgreSQL_INCLUDE_DIR})
  endif()
endif()

if(WITH_MYSQL)
  find_package(MySQL)
  if(MySQL_FOUND)
    add_library(MYSQL INTERFACE IMPORTED GLOBAL)
    set_property(TARGET MYSQL PROPERTY INTERFACE_LINK_LIBRARIES
                                       ${MySQL_LIBRARY})
    set_property(TARGET MYSQL PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                                       ${MySQL_INCLUDE_DIRS})
  endif()
endif()

find_package(PDAL QUIET)
if(PDAL_FOUND)
  add_library(PDAL INTERFACE IMPORTED GLOBAL)
  set_property(TARGET PDAL PROPERTY INTERFACE_LINK_LIBRARIES ${PDAL_LIBRARIES})
  set_property(TARGET PDAL PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                                    ${PDAL_INCLUDE_DIRS})
endif()

if(WITH_LIBLAS)
  find_package(LibLAS)
  if(LibLAS_FOUND)
    add_library(LIBLAS INTERFACE IMPORTED GLOBAL)
    set_property(TARGET LIBLAS PROPERTY INTERFACE_LINK_LIBRARIES
                                        ${LibLAS_C_LIBRARY})
    set_property(TARGET LIBLAS PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                                        ${LibLAS_INCLUDE_DIR})
  endif()
endif()

find_package(NetCDF QUIET)
if(NETCDF_FOUND)
  add_library(NETCDF INTERFACE IMPORTED GLOBAL)
  set_property(TARGET NETCDF PROPERTY INTERFACE_LINK_LIBRARIES
                                      ${NetCDF_LIBRARY})
  set_property(TARGET NETCDF PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                                      ${NetCDF_INCLUDE_DIR})
endif()

find_package(GEOS REQUIRED)
if(GEOS_FOUND)
  add_library(GEOS INTERFACE IMPORTED GLOBAL)
  set_property(TARGET GEOS PROPERTY INTERFACE_LINK_LIBRARIES
                                    ${GEOS_C_LIBRARY${find_library_suffix}})
  set_property(TARGET GEOS PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                                    ${GEOS_INCLUDE_DIR})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)
if(THREADS_FOUND)
  add_library(PTHREAD INTERFACE IMPORTED GLOBAL)
  if(THREADS_HAVE_PTHREAD_ARG)
    set_property(TARGET PTHREAD PROPERTY INTERFACE_COMPILE_OPTIONS "-pthread")
  endif()
  if(CMAKE_THREAD_LIBS_INIT)
    set_property(TARGET PTHREAD PROPERTY INTERFACE_LINK_LIBRARIES
                                         "${CMAKE_THREAD_LIBS_INIT}")
  endif()
endif()

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

find_package(Python3 REQUIRED)
if(PYTHON3_FOUND)
  set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
  #[[
  find_package(PythonLibs REQUIRED)
  find_package(Numpy)
  #]]
endif()

# no target ATLAS in thirdpary/CMakeLists.txt
check_target(ATLAS HAVE_LIBATLAS)

# set(CMAKE_REQUIRED_INCLUDES "${FFTW_INCLUDE_DIR}")
check_target(ICONV HAVE_ICONV_H)
check_target(BZIP2 HAVE_BZLIB_H)
check_target(ZLIB HAVE_ZLIB_H)
check_target(LIBJPEG HAVE_JPEGLIB_H)
check_target(LIBPNG HAVE_PNG_H)
check_target(TIFF HAVE_TIFFIO_H)
check_target(GEOS HAVE_GEOS)
check_target(GDAL HAVE_GDAL)
check_target(GDAL HAVE_OGR)
check_target(SQLITE HAVE_SQLITE)
check_target(POSTGRES HAVE_POSTGRES)
check_target(MYSQL HAVE_MYSQL_H)

check_target(PROJ HAVE_PROJ_H)

check_target(BLAS HAVE_LIBBLAS)
check_target(BLAS HAVE_CBLAS_H)

check_target(LAPACK HAVE_LIBLAPACK)
check_target(LAPACK HAVE_CLAPACK_H)

check_target(FREETYPE HAVE_FT2BUILD_H)
check_target(ODBC HAVE_SQL_H)

if(TARGET POSTGRES)
  try_compile(
    HAVE_PQCMDTUPLES ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/cmake/tests/have_pqcmdtuples.c
    CMAKE_FLAGS "-DINCLUDE_DIRECTORIES:PATH=${PostgreSQL_INCLUDE_DIR}" "-w"
                "-DLINK_LIBRARIES:STRING=${PostgreSQL_LIBRARY}"
    OUTPUT_VARIABLE COMPILE_HAVE_PQCMDTUPLES)
  if(NOT COMPILE_HAVE_PQCMDTUPLES)
    message(
      "Performing Test HAVE_PQCMDTUPLES - Failed\n COMPILE_OUTPUT:${COMPILE_HAVE_PQCMDTUPLES}\n"
    )
  else()
    message(STATUS "Performing Test HAVE_PQCMDTUPLES - Success")
    set(HAVE_PQCMDTUPLES 1)
  endif()
endif()

if(MSVC)
  check_target(PCRE HAVE_PCRE_H)
endif()

check_target(POSTGRES HAVE_LIBPQ_FE_H)

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
