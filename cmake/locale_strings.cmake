
set(ENV{GISRC} "${BIN_DIR}/demolocation/.grassrc${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}")
set(ENV{GISBASE} "${BIN_DIR}")
set(ENV{PATH} "${BIN_DIR}/bin:${BIN_DIR}/scripts:$ENV{PATH}")
set(ENV{PYTHONPATH} "${BIN_DIR}/gui/wxpython:${BIN_DIR}/etc/python:$ENV{PYTHONPATH}")
set(ENV{LD_LIBRARY_PATH} "${BIN_DIR}/lib:$ENV{LD_LIBRARY_PATH}")
set(ENV{LC_ALL} C)


execute_process(COMMAND ${BIN_DIR}/bin/g.parser -t ${INPUT_FILE}
  OUTPUT_VARIABLE run_grass_OV
  ERROR_VARIABLE run_grass_EV
  #OUTPUT_FILE /tmp/tt.out
  RESULT_VARIABLE run_grass_RV
  )

string(REGEX REPLACE "\n" ";" varname "${run_grass_OV}")
set(output_to_write)

#message("run_grass_OV = ${run_grass_OV}")

foreach(line ${varname})
  string(REPLACE "\"" "\\\"" line "${line}")
  set(line "_(\"${line}\")")
  #message("line=${line}")
  list(APPEND output_to_write "${line}")
endforeach()

string(REGEX REPLACE ";" "\n" output_to_write "${output_to_write}")


file(WRITE "${OUTPUT_FILE}" "${output_to_write}\n")
