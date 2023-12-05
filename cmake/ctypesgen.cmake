# AUTHOR(S): Rashad Kanavath <rashad km gmail>
# PURPOSE: 	 Cmake building of lib/python/ctypes (TODO)
# COPYRIGHT: (C) 2020 by the GRASS Development Team
#   	    	 This program is free software under the GPL (>=v2)
#   	    	 Read the file COPYING that comes with GRASS for details.

set(ENV{GISRC} "${BIN_DIR}/demolocation/.grassrc${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}")
set(ENV{GISBASE} "${BIN_DIR}")
set(ENV{PATH} "${BIN_DIR}/bin:${BIN_DIR}/scripts:$ENV{PATH}")
set(ENV{PYTHONPATH} "${BIN_DIR}/gui/wxpython:${BIN_DIR}/etc/python:$ENV{PYTHONPATH}")
if(NOT MSVC)
  set(ENV{LD_LIBRARY_PATH} "${BIN_DIR}/lib:$ENV{LD_LIBRARY_PATH}")
endif()
set(ENV{LC_ALL} C)

set(LIBRARIES)
foreach(LIB ${LIBS})
  if(WIN32)
    list(APPEND LIBRARIES "--library=${BIN_DIR}/lib/${LIB}.dll")
  elseif(APPLE)
    list(APPEND LIBRARIES "--library=${BIN_DIR}/lib/lib${LIB}.so")
  else()
    #This can be linux or unix
    list(APPEND LIBRARIES "--library=${BIN_DIR}/lib/lib${LIB}.so")
  endif()
endforeach()

set(HEADERS)
foreach(HDR ${HDRS})
  list(APPEND HEADERS "${BIN_DIR}/include/grass/${HDR}")
endforeach()

foreach(req  OUT_FILE HDRS LIBS  CTYPESGEN_PY COMPILER )
  if(NOT DEFINED ${req} OR "${${req}}" STREQUAL "")
    message(FATAL_ERROR "you must set ${req}")
  endif()
endforeach()

if(MSVC)
  set(CTYPESFLAGS "${COMPILER} -E -DPACKAGE=\"grasslibs\"")
else()
  set(CTYPESFLAGS "${COMPILER} -E -DPACKAGE=\"grasslibs\" -D__GLIBC_HAVE_LONG_LONG")
endif()

message(STATUS "Running ${PYTHON_EXECUTABLE} ${CTYPESGEN_PY} --cpp=${CTYPESFLAGS} --includedir=\"${BIN_DIR}/include\" --runtime-libdir=\"${BIN_DIR}/lib\" ${HEADERS} ${LIBRARIES} --output=${OUT_FILE}")
execute_process(
  COMMAND ${PYTHON_EXECUTABLE} ${CTYPESGEN_PY}
  --cpp=${CTYPESFLAGS}
  --includedir="${BIN_DIR}/include"
  --runtime-libdir="${BIN_DIR}/lib"
  ${HEADERS}
  ${LIBRARIES}
  --output=${OUT_FILE}
  OUTPUT_VARIABLE ctypesgen_OV
  ERROR_VARIABLE ctypesgen_EV
  RESULT_VARIABLE ctypesgen_RV
  )

if( ctypesgen_RV )
  message(FATAL_ERROR "ctypesgen.py: ${ctypesgen_EV} \n ${ctypesgen_OV}")
endif()
