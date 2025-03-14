include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckCSourceCompiles)

check_include_file(limits.h HAVE_LIMITS_H)
check_include_file(termio.h HAVE_TERMIO_H)
check_include_file(termios.h HAVE_TERMIOS_H)
if(NOT MSVC)
  check_include_file(unistd.h HAVE_UNISTD_H)
else()
  # unistd.h in stocked in thirdparty/msvc/
  set(HAVE_UNISTD_H 1)
endif()
check_include_file(values.h HAVE_VALUES_H)
check_include_file(sys/ioctl.h HAVE_SYS_IOCTL_H)
check_include_file(sys/mtio.h HAVE_SYS_MTIO_H)
check_include_file(sys/resource.h HAVE_SYS_RESOURCE_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(time.h HAVE_TIME_H)
check_include_file(sys/timeb.h HAVE_SYS_TIMEB_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(sys/utsname.h HAVE_SYS_UTSNAME_H)
check_include_file(g2c.h HAVE_G2C_H)
check_include_file(f2c.h HAVE_F2C_H)
check_include_file(cblas.h HAVE_CBLAS_H)
check_include_file(cblas-atlas.h HAVE_CBLAS_ATLAS_H)


if(MSVC)
  set(HAVE_PTHREAD_H 0)
  set(HAVE_REGEX_H 0)
  set(HAVE_LIBINTL_H 0)
  set(HAVE_LANGINFO_H 0)
  set(HAVE_DBM_H 0)
else()
  check_include_file(pthread.h HAVE_PTHREAD_H)
  check_include_file(regex.h HAVE_REGEX_H)
  check_include_file(libintl.h HAVE_LIBINTL_H)
  check_include_file(langinfo.h HAVE_LANGINFO_H)
  check_include_file(dbm.h HAVE_DBM_H)
endif()
#
# # set(CMAKE_REQUIRED_INCLUDES "${FFTW_INCLUDE_DIR}") check_target(ICONV
# HAVE_ICONV_H) check_target(BZIP2 HAVE_BZLIB_H) check_target(ZLIB HAVE_ZLIB_H)
# check_target(LIBJPEG HAVE_JPEGLIB_H) check_target(LIBPNG HAVE_PNG_H)
# check_target(TIFF HAVE_TIFFIO_H) check_target(GEOS HAVE_GEOS)
# check_target(GDAL HAVE_GDAL) check_target(GDAL HAVE_OGR) check_target(SQLITE
# HAVE_SQLITE)
#
# check_target(PROJ HAVE_PROJ_H)
#
# check_target(FREETYPE HAVE_FT2BUILD_H) check_target(POSTGRES HAVE_POSTGRES)
# check_target(ODBC HAVE_SQL_H)
#
# if(MSVC) check_target(PCRE HAVE_PCRE_H) endif()
#
# check_target(POSTGRES HAVE_LIBPQ_FE_H)

# Whether or not we are using G_socks for display communications
set(USE_G_SOCKS 0)

if(WITH_LARGEFILES)
  set(HAVE_LARGEFILES 1)
else()
  set(HAVE_LARGEFILES 0)
endif()

if(MSVC)
  set(GID_TYPE int)
  set(UID_TYPE int)
  set(UID_TYPE int)
  set(RETSIGTYPE "void")
else()
  set(RETSIGTYPE "int")
endif()

# #######################TODO########################
# no target ATLAS in thirdpary/CMakeLists.txt check_target(ATLAS HAVE_LIBATLAS)

set(USE_NLS 0)
if(WITH_NLS)
  set(USE_NLS 1)
endif()

if(MSVC)
  set(PID_TYPE int)
endif()

set(_OE_SOCKETS 0)
set(USE_DELTA_FOR_TZ 0)
set(_REENTRANT 0)
# #######################TODO########################

set(X_DISPLAY_MISSING 1)
if(TARGET X11::X11)
  set(X_DISPLAY_MISSING 0)
endif()

# used in config.cmake.in
set(STATIC_BUILD 0)
if(NOT BUILD_SHARED_LIBS)
  set(STATIC_BUILD 1)
endif()

# used in config.cmake.in
set(GDEBUG 0)
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  set(GDEBUG 1)
endif()

set(CMAKE_REQUIRED_FLAGS ${CMAKE_C_FLAGS})

check_c_source_compiles(
  "
int main(int argc, char *argv[])
{
long long int x;
return 0;
}
"
  HAVE_LONG_LONG_INT)

check_c_source_compiles(
  "
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
int main() {
struct tm *tp;
; return 0; }
"
  TIME_WITH_SYS_TIME)

check_symbol_exists(gethostname "unistd.h" HAVE_GETHOSTNAME)
check_symbol_exists(gettimeofday "sys/time.h" HAVE_GETTIMEOFDAY)
check_symbol_exists(time "time.h" HAVE_TIME)
check_symbol_exists(putenv "stdlib.h" HAVE_PUTENV)
check_symbol_exists(setenv "stdlib.h" HAVE_SETENV)
check_symbol_exists(socket "sys/socket.h" HAVE_SOCKET)
check_symbol_exists(ftime "sys/timeb.h" HAVE_FTIME)
check_symbol_exists(lseek "unistd.h" HAVE_LSEEK)
check_symbol_exists(uname "sys/utsname.h" HAVE_UNAME)
check_symbol_exists(seteuid "unistd.h" HAVE_SETEUID)
check_symbol_exists(setpriority "sys/resource.h" HAVE_SETPRIORITY)
check_symbol_exists(setreuid "unistd.h" HAVE_SETREUID)
check_symbol_exists(setruid "unistd.h" HAVE_SETRUID)
check_symbol_exists(setpgrp "unistd.h" SETPGRP_VOID)
check_symbol_exists(drand48 "stdlib.h" HAVE_DRAND48)
check_symbol_exists(nanosleep "time.h" HAVE_NANOSLEEP)
check_symbol_exists(fseeko "stdio.h" HAVE_FSEEKO)

function(check_symbol_definitions)
  cmake_parse_arguments(PARSE_ARGV 0 ARG "" "SYMBOL" "INCLUDES;DEFINITIONS")

  string(TOUPPER "HAVE_${ARG_SYMBOL}" var_name)

  # First try with a simple check
  check_symbol_exists("${ARG_SYMBOL}" "${ARG_INCLUDES}" "${var_name}")

  if($CACHE{${var_name}})
    return()
  endif()

  # Otherwise, start trying alternatives
  foreach(def IN LISTS ARG_DEFINITIONS)
    unset(${var_name} CACHE)
    set(CMAKE_REQUIRED_DEFINITIONS "-D${def}")
    check_symbol_exists("${ARG_SYMBOL}" "${ARG_INCLUDES}" "${var_name}")
    if($CACHE{${var_name}})
      return()
    endif()
  endforeach()
endfunction()

check_symbol_definitions(
  SYMBOL
  asprintf
  INCLUDES
  stdio.h
  DEFINITIONS
  _GNU_SOURCE
  _BSD_SOURCE)

# set(HAVE_PBUFFERS 0) set(HAVE_PIXMAPS 0) if(WITH_OPENGL) try_compile(
# HAVE_PBUFFERS ${CMAKE_CURRENT_BINARY_DIR}
# ${CMAKE_SOURCE_DIR}/cmake/tests/have_pbuffer.c CMAKE_FLAGS
# "-DINCLUDE_DIRECTORIES:PATH=${OPENGL_INCLUDE_DIR}" "-w"
# "-DLINK_LIBRARIES:STRING=${OPENGL_LIBRARIES}" OUTPUT_VARIABLE
# COMPILE_HAVE_PBUFFERS) if(NOT COMPILE_HAVE_PBUFFERS) message( FATAL_ERROR
# "Performing Test HAVE_PBUFFERS - Failed\n
# COMPILE_OUTPUT:${COMPILE_HAVE_PBUFFERS}\n" ) else() message(STATUS "Performing
# Test HAVE_PBUFFERS - Success") set(HAVE_PBUFFERS 1) endif()
#
# try_compile( HAVE_PIXMAPS ${CMAKE_CURRENT_BINARY_DIR}
# ${CMAKE_SOURCE_DIR}/cmake/tests/have_pixmaps.c CMAKE_FLAGS
# "-DINCLUDE_DIRECTORIES:PATH=${OPENGL_INCLUDE_DIR}" "-w"
# "-DLINK_LIBRARIES:STRING=${OPENGL_LIBRARIES}" OUTPUT_VARIABLE
# COMPILE_HAVE_PIXMAPS)
#
# if(NOT COMPILE_HAVE_PIXMAPS) message( FATAL_ERROR "Performing Test
# HAVE_PIXMAPS - Failed\n COMPILE_OUTPUT:${COMPILE_HAVE_PIXMAPS}\n" ) else()
# message(STATUS "Performing Test HAVE_PIXMAPS - Success") set(HAVE_PIXMAPS 1)
# endif()
#
# endif(WITH_OPENGL)
#
# set(OPENGL_X11 0) set(OPENGL_AQUA 0) set(OPENGL_WINDOWS 0) if(WITH_OPENGL)
# if(APPLE) set(OPENGL_AQUA 1) set(OPENGL_AGL 1) elseif(WIN32)
# set(OPENGL_WINDOWS 1) else() set(OPENGL_X11 1) endif() endif()
