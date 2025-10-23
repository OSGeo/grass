#[[
AUTHOR(S):  Rashad Kanavath <rashad km gmail>
PURPOSE:    create translation strings for grass scripts
            environment. TODO use custom_command POST_BUILD directly
COPYRIGHT:  (C) 2020 by the GRASS Development Team

SPDX-License-Identifier: GPL-2.0-or-later

PARAMS:     BINARY_DIR
            ETCDIR
            GISBASE_DIR
            GISRC
            GUIDIR
            G_NAME
            LIBDIR
            OUTPUT_FILE
            PYDIR
            SCRIPTDIR
            SOURCE_DIR
#]]

file(TO_NATIVE_PATH "${SOURCE_DIR}" MODULE_TOPDIR)
file(TO_NATIVE_PATH "${GISBASE_DIR}" GISBASE_NATIVE)
file(TO_NATIVE_PATH "${BINARY_DIR}" BIN_DIR)
file(TO_NATIVE_PATH "${LIBDIR}" LIB_DIR)
file(TO_NATIVE_PATH "${SCRIPTDIR}" SCRIPTS_DIR)
file(TO_NATIVE_PATH "${GISRC}" GISRC)
file(TO_NATIVE_PATH "${PYDIR}" ETC_PYTHON_DIR)
file(TO_NATIVE_PATH "${GUIDIR}/wxpython" GUI_WXPYTHON_DIR)

if(WIN32)
  set(sep "\;")
  set(env_path "")
else()
  set(sep ":")
  set(env_path ":$ENV{PATH}")
endif()

set(ENV{GISBASE} "${GISBASE_NATIVE}")
set(ENV{GISRC} ${GISRC})
set(ENV{PATH} "${BIN_DIR}${sep}${SCRIPTS_DIR}${env_path}")
set(ENV{PYTHONPATH}
    "${ETC_PYTHON_DIR}${sep}${GUI_WXPYTHON_DIR}${sep}$ENV{PYTHONPATH}")
if(NOT MSVC)
  set(ENV{LD_LIBRARY_PATH} "${LIB_DIR}${sep}$ENV{LD_LIBRARY_PATH}")
endif()
set(ENV{LC_ALL} C)
set(ENV{LANG} C)
set(ENV{LANGUAGE} C)
set(ENV{MODULE_TOPDIR} ${MODULE_TOPDIR})

set(SCRIPT_EXT "")
if(WIN32)
  set(SCRIPT_EXT ".py")
endif()

if(WIN32)
  set(PGM_NAME ${G_NAME})
  configure_file(${SOURCE_DIR}/cmake/windows_launch.bat.in
                 ${SCRIPTDIR}/${G_NAME}.bat @ONLY)
endif(WIN32)

execute_process(
  COMMAND ${BINARY_DIR}/g.parser -t ${SCRIPTDIR}/${G_NAME}${SCRIPT_EXT}
  OUTPUT_VARIABLE run_g_parser_OV
  ERROR_VARIABLE run_g_parser_EV
  RESULT_VARIABLE run_g_parser_RV)

string(REGEX REPLACE "\n" ";" varname "${run_g_parser_OV}")
set(output_to_write)
foreach(line ${varname})
  string(REPLACE "\"" "\\\"" line "${line}")
  set(line "_(\"${line}\")")
  list(APPEND output_to_write "${line}")
endforeach()

string(REGEX REPLACE ";" "\n" output_to_write "${output_to_write}")
file(WRITE "${OUTPUT_FILE}" "${output_to_write}\n")
