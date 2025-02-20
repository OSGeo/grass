#[[
AUTHOR(S):  Rashad Kanavath <rashad km gmail>
PURPOSE:    Cmake building of lib/python/ctypes (TODO)
COPYRIGHT:  (C) 2020 by the GRASS Development Team

SPDX-License-Identifier: GPL-2.0-or-later
#]]

set(ENV{GISRC}
    "${OUTDIR}/${GRASS_INSTALL_DEMODIR}/.grassrc${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}"
)
set(ENV{GISBASE} "${OUTDIR}/${GISBASE_DIR}")
set(ENV{PATH}
    "${OUTDIR}/${GRASS_INSTALL_BINDIR}:${OUTDIR}/${GRASS_INSTALL_SCRIPTDIR}:$ENV{PATH}"
)
set(ENV{PYTHONPATH}
    "${OUTDIR}/${GRASS_INSTALL_GUIDIR}/wxpython:${OUTDIR}/${GRASS_INSTALL_PYDIR}:$ENV{PYTHONPATH}"
)
if(NOT MSVC)
  set(ENV{LD_LIBRARY_PATH}
      "${OUTDIR}/${GRASS_INSTALL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")
endif()
set(ENV{LC_ALL} C)

set(LIBRARIES)
foreach(LIB ${LIBS})
  list(APPEND LIBRARIES --library=${LIB})
endforeach()

set(HEADERS)
foreach(HDR ${HDRS})
  list(APPEND HEADERS "${OUTDIR}/${GRASS_INSTALL_INCLUDEDIR}/grass/${HDR}")
endforeach()

foreach(req OUT_FILE HDRS LIBS CTYPESGEN_PY COMPILER)
  if(NOT DEFINED ${req} OR "${${req}}" STREQUAL "")
    message(FATAL_ERROR "you must set ${req}")
  endif()
endforeach()

if(MSVC)
  set(CTYPESFLAGS "${COMPILER} -E -DPACKAGE=\"grasslibs\"")
else()
  set(CTYPESFLAGS
      "${COMPILER} -E -DPACKAGE=\"grasslibs\" -D__GLIBC_HAVE_LONG_LONG")
endif()

message(
  STATUS
    "Running ${PYTHON_EXECUTABLE} ${CTYPESGEN_PY} --cpp=${CTYPESFLAGS} --no-embed-preamble --strip-build-path ${RUNTIME_GISBASE} --includedir=\"${OUTDIR}/${GRASS_INSTALL_INCLUDEDIR}\" ${LIBRARIES} ${HEADERS} --output=${OUT_FILE}"
)
execute_process(
  COMMAND
    ${PYTHON_EXECUTABLE} ${CTYPESGEN_PY} --cpp=${CTYPESFLAGS}
    --no-embed-preamble --strip-build-path ${RUNTIME_GISBASE}
    --includedir="${OUTDIR}/${GRASS_INSTALL_INCLUDEDIR}" ${LIBRARIES} ${HEADERS}
    --output=${OUT_FILE}
  OUTPUT_VARIABLE ctypesgen_OV
  ERROR_VARIABLE ctypesgen_EV
  RESULT_VARIABLE ctypesgen_RV)
if(ctypesgen_RV)
  message(FATAL_ERROR "ctypesgen.py: ${ctypesgen_EV} \n ${ctypesgen_OV}")
endif()
