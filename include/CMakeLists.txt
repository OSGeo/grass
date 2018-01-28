include(CheckIncludeFile)
check_include_file(limits.h HAVE_LIMITS_H)
check_include_file(termio.h HAVE_TERMIO_H)
check_include_file(termios.h HAVE_TERMIOS_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(values.h HAVE_VALUES_H)
check_include_file(zlib.h HAVE_ZLIB_H)
if(TARGET BZIP2)
  #check_include_file(bzlib.h HAVE_BZLIB_H)
    set(HAVE_BZLIB_H 1)
endif()
check_include_file(sys/ioctl.h HAVE_SYS_IOCTL_H)
check_include_file(sys/mtio.h HAVE_SYS_MTIO_H)
check_include_file(sys/resource.h HAVE_SYS_RESOURCE_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(time.h TIME_WITH_SYS_TIME)
check_include_file(sys/timeb.h HAVE_SYS_TIMEB_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(sys/utsname.h HAVE_SYS_UTSNAME_H)
check_include_file(g2c.h HAVE_G2C_H)
check_include_file(f2c.h HAVE_F2C_H)
check_include_file(cblas.h HAVE_CBLAS_H)
check_include_file(clapack.h HAVE_CLAPACK_H)
check_include_file(langinfo.h HAVE_LANGINFO_H)

if(TARGET FREETYPE)
  #check_include_file(ft2build.h HAVE_FT2BUILD_H)
  set(HAVE_FT2BUILD_H 1)
endif()  

set(CMAKE_REQUIRED_INCLUDES "${FFTW_INCLUDE_DIR}")
check_include_file(pthread.h HAVE_PTHREAD_H)
check_include_file(regex.h HAVE_REGEX_H)
check_include_file(iconv.h HAVE_ICONV_H)
check_include_file(libintl.h HAVE_LIBINTL_H)
check_include_file(dbm.h HAVE_DBM_H)

check_include_file(dfftw.h HAVE_DFFTW_H)
check_include_file(fftw3.h HAVE_FFTW3_H)
if(NOT HAVE_FFTW3_H)
  check_include_file(fftw.h HAVE_FFTW_H)
endif()

set(HAVE_JPEGLIB_H 0)
if(TARGET LIBJPEG)
  set(HAVE_JPEGLIB_H 1)
  #check_include_file(jpeglib.h HAVE_JPEGLIB_H)
endif()

set(HAVE_PNG_H 0)
if(TARGET LIBPNG)
  set(HAVE_PNG_H 1)
  #check_include_file(png.h HAVE_PNG_H)
endif()

set(HAVE_TIFFIO_H 0)
if(TARGET TIFF)
  set(HAVE_TIFFIO_H 1)
  #check_include_file(tiffio.h HAVE_TIFFIO_H)
endif()

set(HAVE_GEOS 0)
if(TARGET GEOS)
  set(HAVE_GEOS 1)
endif()

set(HAVE_GDAL 0)
set(HAVE_OGR  0)
if(TARGET GDAL)
  set(HAVE_GDAL 1)
  set(HAVE_OGR 1)
endif()

set(HAVE_SQLITE 0)
if(TARGET SQLITE)
  set(HAVE_SQLITE 1)
endif()

if(WITH_NLS)
  set(USE_NLS 1)
endif()

set(HAVE_LIBBLAS 0)
if(TARGET BLAS)
  set(HAVE_LIBBLAS 1)
endif()

set(HAVE_LIBLAPACK 0)
if(TARGET LAPACK)
  set(HAVE_LIBLAPACK 1)
endif()

########################TODO########################
set(HAVE_POSTGRES 0)
  set(HAVE_LIBPQ_FE_H 0)
  set(HAVE_PQCMDTUPLES 0)
if(TARGET POSTGRES)
  set(HAVE_POSTGRES 1)
  set(HAVE_LIBPQ_FE_H 0)
  set(HAVE_PQCMDTUPLES 0)
endif()

set(HAVE_LIBATLAS 0)
if(TARGET ATLAS)
  set(HAVE_LIBATLAS 1)
endif()

set(HAVE_READLINE_READLINE_H 0)
set(HAVE_SQL_H 0)
set(USE_NLS 0)

set(USE_G_SOCKS 0)
set(RETSIGTYPE_RESULT "int")

set(gid_t 0)
set(uid_t 0)
set(off_t 0)




set(_OE_SOCKETS 0)
set(USE_DELTA_FOR_TZ 0)
set(_REENTRANT 0)
########################TODO########################

set(X_DISPLAY_MISSING 1)
if(TARGET X11)
  set(X_DISPLAY_MISSING 0)
endif()

#used in config.cmake.in
set(STATIC_BUILD TRUE)
if(BUILD_SHARED_LIBS)
  set(STATIC_BUILD FALSE)
endif()

#used in config.cmake.in
set(GDEBUG 0) 
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
  set(GDEBUG 1)
endif()

include(CheckCSourceCompiles)

set(CMAKE_REQUIRED_FLAGS ${CMAKE_C_FLAGS})

unset(HAVE_LONG_LONG_INT CACHE)

CHECK_C_SOURCE_COMPILES("
int main(int argc, char *argv[])
{
long long int x;
return 0;
}
"
HAVE_LONG_LONG_INT )

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
#message(STATUS "Copying headers to ${CMAKE_BINARY_DIR}/include/grass/")

execute_process(COMMAND ${CMAKE_COMMAND}
  -E make_directory
  ${CMAKE_BINARY_DIR}/include/grass/)

file(GLOB_RECURSE SRCHS "*.h")
foreach(srch ${SRCHS})
  get_filename_component(srch_PATH ${srch} PATH)
  STRING(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" dsth_PATH "${srch_PATH}")
  file(COPY ${srch} DESTINATION ${CMAKE_BINARY_DIR}/include/grass${dsth_PATH})
endforeach()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
  "${CMAKE_CURRENT_BINARY_DIR}/grass/version.h")

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
