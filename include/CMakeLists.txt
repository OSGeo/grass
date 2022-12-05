include(CheckIncludeFile)
include(CheckFunctionExists) #TODO: use CheckSymbolExists
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

#set(CMAKE_REQUIRED_INCLUDES "${FFTW_INCLUDE_DIR}")
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

#TODO: check with more version of proj

if(PROJ_VERSION_STRING GREATER 599 AND PROJ_VERSION_STRING LESS 700)
	message(STATUS "Using PROJ.4 API version 6.x")
	set(PROJMAJOR 6)
	check_target(PROJ HAVE_PROJ_H)
elseif(PROJ_VERSION_STRING GREATER 499 AND PROJ_VERSION_STRING LESS 600)
	check_target(PROJ HAVE_PROJ_H)
	message(STATUS "Using PROJ.4 API version 5.x")
	set(PROJMAJOR 5)
elseif(PROJ_VERSION_STRING GREATER 399 AND PROJ_VERSION_STRING LESS 500)
	set(USE_PROJ4API 1)
	message(STATUS "Using PROJ.4 API version 4.x")
	set(PROJMAJOR 4)
endif()

check_target(BLAS HAVE_LIBBLAS)
check_target(BLAS HAVE_CBLAS_H)

check_target(LAPACK HAVE_LIBLAPACK)
check_target(LAPACK HAVE_CLAPACK_H)

check_target(FREETYPE HAVE_FT2BUILD_H)
check_target(POSTGRES HAVE_POSTGRES)
check_target(ODBC HAVE_SQL_H)

if(TARGET POSTGRES)
	try_compile(HAVE_PQCMDTUPLES
		${CMAKE_CURRENT_BINARY_DIR}
		${CMAKE_SOURCE_DIR}/cmake/tests/have_pqcmdtuples.c
		CMAKE_FLAGS "-DINCLUDE_DIRECTORIES:PATH=${PostgreSQL_INCLUDE_DIR}"
					"-w"
					"-DLINK_LIBRARIES:STRING=${PostgreSQL_LIBRARY}"
		OUTPUT_VARIABLE COMPILE_HAVE_PQCMDTUPLES)
	if(NOT COMPILE_HAVE_PQCMDTUPLES)
		message("Performing Test HAVE_PQCMDTUPLES - Failed\n COMPILE_OUTPUT:${COMPILE_HAVE_PQCMDTUPLES}\n")
	else()
		message(STATUS "Performing Test HAVE_PQCMDTUPLES - Success")
		set(HAVE_PQCMDTUPLES 1)
	endif()
endif()

if(MSVC)
check_target(PCRE HAVE_PCRE_H)
endif()

check_target(POSTGRES HAVE_LIBPQ_FE_H)

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

########################TODO########################
#no target ATLAS in thirdpary/CMakeLists.txt
check_target(ATLAS HAVE_LIBATLAS)

set(USE_NLS 0)
if(WITH_NLS)
  set(USE_NLS 1)
endif()
set(HAVE_READLINE_READLINE_H 0)

if(MSVC)
set(PID_TYPE int)
endif()

set(_OE_SOCKETS 0)
set(USE_DELTA_FOR_TZ 0)
set(_REENTRANT 0)
########################TODO########################

set(X_DISPLAY_MISSING 1)
if(TARGET X11)
  set(X_DISPLAY_MISSING 0)
endif()

#used in config.cmake.in
set(STATIC_BUILD 0)
if(NOT BUILD_SHARED_LIBS)
  set(STATIC_BUILD 1)
endif()

#used in config.cmake.in
set(GDEBUG 0)
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
  set(GDEBUG 1)
endif()

include(CheckCSourceCompiles)

set(CMAKE_REQUIRED_FLAGS ${CMAKE_C_FLAGS})

CHECK_C_SOURCE_COMPILES("
int main(int argc, char *argv[])
{
long long int x;
return 0;
}
"
HAVE_LONG_LONG_INT)

CHECK_C_SOURCE_COMPILES("
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
int main() {
struct tm *tp;
; return 0; }
"
TIME_WITH_SYS_TIME)

CHECK_FUNCTION_EXISTS(ftime HAVE_FTIME)
CHECK_FUNCTION_EXISTS(gethostname HAVE_GETHOSTNAME)
CHECK_FUNCTION_EXISTS(gettimeofday HAVE_GETTIMEOFDAY)
CHECK_FUNCTION_EXISTS(lseek HAVE_LSEEK)
CHECK_FUNCTION_EXISTS(time HAVE_TIME)
CHECK_FUNCTION_EXISTS(uname HAVE_UNAME)
CHECK_FUNCTION_EXISTS(seteuid HAVE_SETEUID)
CHECK_FUNCTION_EXISTS(setpriority HAVE_SETPRIORITY)
CHECK_FUNCTION_EXISTS(setreuid HAVE_SETREUID)
CHECK_FUNCTION_EXISTS(setruid HAVE_SETRUID)
CHECK_FUNCTION_EXISTS(setpgrp SETPGRP_VOID)
CHECK_FUNCTION_EXISTS(drand48 HAVE_DRAND48)
CHECK_FUNCTION_EXISTS(nanosleep HAVE_NANOSLEEP)
CHECK_FUNCTION_EXISTS(asprintf HAVE_ASPRINTF)
CHECK_FUNCTION_EXISTS(putenv HAVE_PUTENV)
CHECK_FUNCTION_EXISTS(setenv HAVE_SETENV)
CHECK_FUNCTION_EXISTS(socket HAVE_SOCKET)

set(HAVE_PBUFFERS 0)
set(HAVE_PIXMAPS 0)
if(WITH_OPENGL)
try_compile(HAVE_PBUFFERS ${CMAKE_CURRENT_BINARY_DIR}
${CMAKE_SOURCE_DIR}/cmake/tests/have_pbuffer.c
CMAKE_FLAGS "-DINCLUDE_DIRECTORIES:PATH=${OPENGL_INCLUDE_DIR}"
            "-w"
	    "-DLINK_LIBRARIES:STRING=${OPENGL_LIBRARIES}"
OUTPUT_VARIABLE COMPILE_HAVE_PBUFFERS
)
if(NOT COMPILE_HAVE_PBUFFERS)
  message(FATAL_ERROR "Performing Test HAVE_PBUFFERS - Failed\n COMPILE_OUTPUT:${COMPILE_HAVE_PBUFFERS}\n")
else()
  message(STATUS "Performing Test HAVE_PBUFFERS - Success")
  set(HAVE_PBUFFERS 1)
endif()


try_compile(HAVE_PIXMAPS ${CMAKE_CURRENT_BINARY_DIR}
${CMAKE_SOURCE_DIR}/cmake/tests/have_pixmaps.c
CMAKE_FLAGS "-DINCLUDE_DIRECTORIES:PATH=${OPENGL_INCLUDE_DIR}"
            "-w"
	    "-DLINK_LIBRARIES:STRING=${OPENGL_LIBRARIES}"
 OUTPUT_VARIABLE COMPILE_HAVE_PIXMAPS
 )

if(NOT COMPILE_HAVE_PIXMAPS)
  message(FATAL_ERROR "Performing Test HAVE_PIXMAPS - Failed\n COMPILE_OUTPUT:${COMPILE_HAVE_PIXMAPS}\n")
else()
  message(STATUS "Performing Test HAVE_PIXMAPS - Success")
  set(HAVE_PIXMAPS 1)
endif()

endif(WITH_OPENGL)

CHECK_FUNCTION_EXISTS(fseeko HAVE_FSEEKO)


set(OPENGL_X11 0)
set(OPENGL_AQUA 0)
set(OPENGL_WINDOWS 0)
if(WITH_OPENGL)
  if(APPLE)
    set(OPENGL_AQUA 1)
  elseif(WIN32)
    set(OPENGL_WINDOWS 1)
  else()
    set(OPENGL_X11 1)
  endif()
endif()

file(GLOB_RECURSE SRCHS "*.h")
 set(include_depends)
foreach(srch ${SRCHS})
  get_filename_component(srch_PATH ${srch} PATH)
  get_filename_component(srch_NAME ${srch} NAME)
  STRING(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" dsth_PATH "${srch_PATH}")
  set(output_dir ${CMAKE_BINARY_DIR}/include/grass${dsth_PATH})

  add_custom_command(
    OUTPUT ${output_dir}/${srch_NAME}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${output_dir}
    COMMAND ${CMAKE_COMMAND} -E copy ${srch} ${output_dir}
    COMMENT "Copy ${srch} to ${CMAKE_BINARY_DIR}/include/grass${dsth_PATH}/${srch_NAME}"
    )
  list(APPEND include_depends ${output_dir}/${srch_NAME} )
endforeach()

add_custom_target(copy_header DEPENDS ${include_depends} LIB_PYTHON)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/grass/version.h.in
  "${CMAKE_CURRENT_BINARY_DIR}/grass/version.h")

message(STATUS "Creating ${CMAKE_CURRENT_BINARY_DIR}/grass/config.h")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/grass/config.h")

file(WRITE ${CMAKE_BINARY_DIR}/include/grass/copying.h "")
file(STRINGS ${CMAKE_SOURCE_DIR}/COPYING copying_lines)
foreach(copying_line ${copying_lines})
  file(APPEND ${CMAKE_BINARY_DIR}/include/grass/copying.h "\"${copying_line} \\n\"\n"    )
endforeach()

file(WRITE ${CMAKE_BINARY_DIR}/include/grass/citing.h "")
file(STRINGS ${CMAKE_SOURCE_DIR}/CITING citing_lines)
foreach(citing_line ${citing_lines})
  file(APPEND ${CMAKE_BINARY_DIR}/include/grass/citing.h "\"${citing_line}\\n\"\n"    )
endforeach()

#TODO
#file(READ  ${CMAKE_SOURCE_DIR}/config.status config_status_header)
file(WRITE ${CMAKE_BINARY_DIR}/include/grass/confparms.h "\"/*  */\\n\""    )
