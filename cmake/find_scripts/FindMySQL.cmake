execute_process(
  COMMAND mysql_config --cflags
  OUTPUT_VARIABLE MySQL_INCLUDE_DIRS
  OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(
  COMMAND mysql_config --libs
  OUTPUT_VARIABLE MySQL_LIBRARY
  OUTPUT_STRIP_TRAILING_WHITESPACE)

if(MySQL_INCLUDE_DIRS AND MySQL_LIBRARY)
  string(REGEX REPLACE "(^| +)-I" " " _dummy ${MySQL_INCLUDE_DIRS})
  string(STRIP ${_dummy} _dummy)
  string(REPLACE " " ";" MySQL_INCLUDE_DIRS ${_dummy})
  message(STATUS "Found MySQL: ${MySQL_LIBRARY}")
  set(MySQL_FOUND TRUE)
endif()
