include(CheckIncludeFile)
check_include_file(limits.h HAVE_LIMITS_H)
check_include_file(termio.h HAVE_TERMIO_H)
check_include_file(termios.h HAVE_TERMIOS_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(values.h HAVE_VALUES_H)
check_include_file(zlib.h HAVE_ZLIB_H)
check_include_file(bzlib.h HAVE_BZLIB_H)
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
check_include_file(ft2build.h HAVE_FT2BUILD_H)
check_include_file(pthread.h HAVE_PTHREAD_H)
check_include_file(regex.h HAVE_REGEX_H)
check_include_file(iconv.h HAVE_ICONV_H)
check_include_file(libintl.h HAVE_LIBINTL_H)
check_include_file(dbm.h HAVE_DBM_H)
check_include_file(dfftw.h HAVE_DFFTW_H)
check_include_file(fftw.h HAVE_FFTW_H)
check_include_file(fftw3.h HAVE_FFTW3_H)
check_include_file(jpeglib.h HAVE_JPEGLIB_H)
check_include_file(png.h HAVE_PNG_H)
check_include_file(tiffio.h HAVE_TIFFIO_H)
#used in config.cmake.in
set(STATIC_BUILD FALSE)
if(BUILD_SHARED_LIBS)
  set(STATIC_BUILD TRUE)
endif()

#used in config.cmake.in
set(GDEBUG 0) 
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
  set(GDEBUG TRUE)
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


file(READ  ${CMAKE_SOURCE_DIR}/COPYING copying_header)
file(WRITE ${CMAKE_BINARY_DIR}/include/grass/copying.h "/* ${copying_header} */"    )

file(READ  ${CMAKE_SOURCE_DIR}/CITING citing_header)
file(WRITE ${CMAKE_BINARY_DIR}/include/grass/citing.h "/* ${citing_header} */"    )

#TODO
#file(READ  ${CMAKE_SOURCE_DIR}/config.status config_status_header)
file(WRITE ${CMAKE_BINARY_DIR}/include/grass/confparms.h "/*  */"    )
