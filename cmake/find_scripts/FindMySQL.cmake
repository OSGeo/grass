execute_process(
  COMMAND mysql_config --cflags
  OUTPUT_VARIABLE MYSQL_INCLUDE_DIRS
  OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(
  COMMAND mysql_config --libs
  OUTPUT_VARIABLE MYSQL_LIBRARY
  OUTPUT_STRIP_TRAILING_WHITESPACE)

if(MYSQL_INCLUDE_DIRS AND MYSQL_LIBRARY)
  string(REGEX REPLACE "(^| +)-I" " " _dummy ${MYSQL_INCLUDE_DIRS})
  string(STRIP ${_dummy} _dummy)
  string(REPLACE " " ";" MYSQL_INCLUDE_DIRS ${_dummy})
  message(STATUS "Found MySQL: ${MYSQL_LIBRARY}")
  set(MYSQL_FOUND TRUE)
endif()
