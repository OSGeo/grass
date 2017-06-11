set(ENV{GISRC} "${BIN_DIR}/demolocation/.grassrc${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}")
set(ENV{GISBASE} "${BIN_DIR}")
set(ENV{PATH} "${BIN_DIR}/bin:${BIN_DIR}/scripts:$ENV{PATH}")
set(ENV{PYTHONPATH} "${BIN_DIR}/gui/wxpython:${BIN_DIR}/etc/python:$ENV{PYTHONPATH}")
set(ENV{LD_LIBRARY_PATH} "${BIN_DIR}/lib:$ENV{LD_LIBRARY_PATH}")
set(ENV{LC_ALL} C)

set(LIBRARIES)
foreach(LIB ${LIBS})
  list(APPEND LIBRARIES "--library=${BIN_DIR}/lib/lib${LIB}.so")
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


#NLS_CFLAGS = -DPACKAGE=\"$(PACKAGE)\"
#USE_LARGEFILES      = 1
#LFS_CFLAGS          = 
#CTYPESFLAGS = --cpp "$(CC) -E $(CPPFLAGS) $(LFS_CFLAGS) $(EXTRA_CFLAGS) $(NLS_CFLAGS) $(DEFS) $(EXTRA_INC) $(INC) -D__GLIBC_HAVE_LONG_LONG"

set(CTYPESFLAGS "${COMPILER} -E -DPACKAGE=\"grasslibs\" -D__GLIBC_HAVE_LONG_LONG")

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

#message(FATAL_ERROR "ctypesgen_RV = ${ctypesgen_OV}")
