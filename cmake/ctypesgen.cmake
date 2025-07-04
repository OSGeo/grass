#[[
AUTHOR(S):  Rashad Kanavath <rashad km gmail>
PURPOSE:    Generation of Python interface to GRASS API
COPYRIGHT:  (c) 2020-2025 by the GRASS Development Team

SPDX-License-Identifier: GPL-2.0-or-later
#]]

foreach(req OUT_FILE HDRS LIBS CTYPESGEN_PY COMPILER)
  if(NOT DEFINED ${req} OR "${${req}}" STREQUAL "")
    message(FATAL_ERROR "ctypesgen.cmake: you must set ${req}")
  endif()
endforeach()

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
  set(_d
      "${OUTDIR}/${GRASS_INSTALL_LIBDIR}:${OUTDIR}/${GRASS_INSTALL_BINDIR}:${OUTDIR}/${GRASS_INSTALL_SCRIPTDIR}"
  )
  set(ENV{${LD_LIBRARY_PATH_VAR}} "${_d}:$ENV{${LD_LIBRARY_PATH_VAR}}")
  unset(_d)
endif()
set(ENV{LC_ALL} C)

set(CTYPESFLAGS "${COMPILER} -E ${C_FLAGS}")

set(LIBRARIES)
foreach(LIB ${LIBS})
  list(APPEND LIBRARIES "-l${LIB}")
endforeach()

set(INC_HEADERS)
foreach(INCHDR ${INCHDRS})
  list(APPEND INC_HEADERS "-I${INCHDR}")
endforeach()

set(DEFINES)
if(MSVC)
  # ??
elseif(APPLE)
  # ??
else()
  list(APPEND DEFINES "__GLIBC_HAVE_LONG_LONG")
endif()
list(TRANSFORM DEFINES PREPEND "-D")

set(HEADERS)
foreach(HDR ${HDRS})
  list(APPEND HEADERS "${OUTDIR}/${GRASS_INSTALL_INCLUDEDIR}/grass/${HDR}")
endforeach()

message(STATUS "Generating ${OUT_FILE}")
execute_process(
  COMMAND
    ${PYTHON_EXECUTABLE} ${CTYPESGEN_PY} --cpp "${CTYPESFLAGS}"
    --no-embed-preamble --strip-build-path ${RUNTIME_GISBASE} ${INC_HEADERS}
    ${LIBRARIES} ${DEFINES} -o ${OUT_FILE} ${HEADERS}
  OUTPUT_VARIABLE ctypesgen_OV
  ERROR_VARIABLE ctypesgen_EV
  RESULT_VARIABLE ctypesgen_RV COMMAND_ECHO STDOUT)

if(ctypesgen_RV)
  message(FATAL_ERROR "${CTYPESGEN_PY}: ${ctypesgen_EV} \n ${ctypesgen_OV}")
endif()
