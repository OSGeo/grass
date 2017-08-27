find_path(FFTW_INCLUDE_DIR fftw3.h)
  
#fftw double lib
find_library(FFTWD_LIB fftw3 )
find_library(FFTWD_THREADS_LIB fftw3_threads) # threads support

set(FFTW_LIBRARIES)

if(FFTWD_LIB)
  set(FFTWD_FOUND 1)
  set(FFTW_LIBRARIES ${FFTWD_LIB})
  if(FFTWD_THREADS_LIB)
    set(FFTW_LIBRARIES "${FFTW_LIBRARIES};${FFTWD_THREADS_LIB}")
  endif()
endif()

#Single Precision
find_library(FFTWF_LIB fftw3f)
find_library(FFTWF_THREADS_LIB fftw3f_threads) #threads support

if(FFTWF_LIB)
  set(FFTWF_FOUND 1)
  set(FFTW_LIBRARIES "${FFTW_LIBRARIES};${FFTWF_LIB}")
  if(FFTWF_THREADS_LIB)
    set(FFTW_LIBRARIES "${FFTW_LIBRARIES};${FFTWF_THREADS_LIB}")
  endif()
endif()
  
if(NOT FFTWD_FOUND  AND NOT FFTWF_FOUND )
  set(FFTW_FOUND FALSE)
endif()

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FFTW
  REQUIRED_VARS FFTW_LIBRARIES FFTW_INCLUDE_DIR)
