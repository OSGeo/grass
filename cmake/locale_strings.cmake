# AUTHOR(S): Rashad Kanavath <rashad km gmail>
# PURPOSE: 	  create translation strings for grass scripts
#             environment. TODO use custom_command POST_BUILD directly
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

execute_process(COMMAND ${BIN_DIR}/bin/g.parser -t ${INPUT_FILE}
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
