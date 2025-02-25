find_path(FFTW_INCLUDE_DIR fftw3.h)

if(FFTW_INCLUDE_DIR)
  set(HAVE_FFTW3_H 1)
  message(STATUS "Found fftw3.h in ${FFTW_INCLUDE_DIR}")
else()
  find_path(FFTW_INCLUDE_DIR fftw.h)
  if(FFTW_INCLUDE_DIR)
    set(HAVE_FFTW_H 1)
    message(STATUS "Found fftw.h in ${FFTW_INCLUDE_DIR}")
  endif()
endif()

find_path(DFFTW_INCLUDE_DIR dfftw.h)
if(DFFTW_INCLUDE_DIR)
  set(HAVE_DFFTW_H 1)
  message(STATUS "Found dfftw.h in ${FFTW_INCLUDE_DIR}")
endif()

# fftw double lib
find_library(FFTWD_LIB fftw3)
find_library(FFTWD_THREADS_LIB fftw3_threads) # threads support

set(FFTW_LIBRARIES)
if(FFTWD_LIB)
  set(FFTWD_FOUND 1)
  set(FFTW_LIBRARIES ${FFTWD_LIB})
  if(FFTWD_THREADS_LIB)
    set(FFTW_LIBRARIES "${FFTW_LIBRARIES};${FFTWD_THREADS_LIB}")
  endif()
endif()

# Single Precision
find_library(FFTWF_LIB fftw3f)
find_library(FFTWF_THREADS_LIB fftw3f_threads) # threads support

if(FFTWF_LIB)
  set(FFTWF_FOUND 1)
  set(FFTW_LIBRARIES "${FFTW_LIBRARIES};${FFTWF_LIB}")
  if(FFTWF_THREADS_LIB)
    set(FFTW_LIBRARIES "${FFTW_LIBRARIES};${FFTWF_THREADS_LIB}")
  endif()
endif()

if(NOT FFTWD_FOUND AND NOT FFTWF_FOUND)
  set(FFTW_FOUND FALSE)
endif()

mark_as_advanced(
  FFTW_LIBRARIES
  FFTW_INCLUDE_DIR
  DFFTW_INCLUDE_DIR
  FFTWF_LIB
  FFTWF_THREADS_LIB
  FFTWD_LIB
  FFTWD_THREADS_LIB)

# copy HAVE_ to parent scope so that we can use in include/CMakeLists.txt and
# rest
set(HAVE_FFTW3_H ${HAVE_FFTW3_H})
set(HAVE_FFTW_H ${HAVE_FFTW_H})
set(HAVE_DFFTW_H ${HAVE_DFFTW_H})
include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
find_package_handle_standard_args(FFTW REQUIRED_VARS FFTW_LIBRARIES
                                                     FFTW_INCLUDE_DIR)
