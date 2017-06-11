# AUTHOR(S): Rashad Kanavath <rashad km gmail>
# PURPOSE: 	  create translation strings for grass scripts
#             environment. TODO use custom_command POST_BUILD directly
# COPYRIGHT: (C) 2020 by the GRASS Development Team
#   	    	 This program is free software under the GPL (>=v2)
#   	    	 Read the file COPYING that comes with GRASS for details.
#-DBINARY_DIR=
#-DG_NAME=
#-DSRC_SCRIPT_FILE=
#-DOUTPUT_FILE=
#-DSOURCE_DIR=

set(GISBASE ${BINARY_DIR}/gisbase)
file(TO_NATIVE_PATH "${GISBASE}" GISBASE_NATIVE)
file(TO_NATIVE_PATH "${GISBASE}/etc/config/rc" GISRC)
file(TO_NATIVE_PATH "${BINARY_DIR}/bin" BINARY_DIR)
file(TO_NATIVE_PATH "${BINARY_DIR}/lib" LIB_DIR)
file(TO_NATIVE_PATH "${SOURCE_DIR}" MODULE_TOPDIR)
file(TO_NATIVE_PATH "${GISBASE}/scripts" SCRIPTS_DIR)
file(TO_NATIVE_PATH "${GISBASE}/etc/python" ETC_PYTHON_DIR)
file(TO_NATIVE_PATH "${GISBASE}/gui/wxpython" GUI_WXPYTHON_DIR)

if(WIN32)
  set(sep "\;")
  set(env_path "")
else()
  set(sep ":")
  set(env_path ":$ENV{PATH}")
endif()

set(ENV{GISBASE} "${GISBASE_NATIVE}")
set(ENV{GISRC} ${GISRC})
set(ENV{PATH} "${BINARY_DIR}${sep}${SCRIPTS_DIR}${env_path}")
set(ENV{PYTHONPATH} "${ETC_PYTHON_DIR}${sep}${GUI_WXPYTHON_DIR}${sep}$ENV{PYTHONPATH}")
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
  configure_file(
    ${SOURCE_DIR}/cmake/windows_launch.bat.in
    ${GISBASE}/scripts/${G_NAME}.bat @ONLY)
endif(WIN32)

set(TMP_SCRIPT_FILE ${BINARY_DIR}/CMakeFiles/${G_NAME}${SCRIPT_EXT})
configure_file(${SRC_SCRIPT_FILE} ${TMP_SCRIPT_FILE} COPYONLY)
file(
  COPY ${TMP_SCRIPT_FILE}
  DESTINATION ${GISBASE}/scripts/
  FILE_PERMISSIONS
  OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_EXECUTE
  WORLD_READ WORLD_EXECUTE)


execute_process(COMMAND
  ${BINARY_DIR}/bin/g.parser -t ${GISBASE}/scripts/${G_NAME}${SCRIPT_EXT}
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
file(REMOVE ${TMP_SCRIPT_FILE})
