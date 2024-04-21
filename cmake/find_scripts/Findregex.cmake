find_path(regex_ROOT_DIR NAMES include/regex.h)
find_path(
  regex_INCLUDE_DIRS
  NAMES regex.h
  HINTS ${regex_ROOT_DIR}/include)
find_library(
  regex_LIBRARIES
  NAMES regex
  HINTS ${regex_ROOT_DIR}/lib)
if(regex_INCLUDE_DIRS)
  set(regex_FOUND TRUE)
  if(regex_LIBRARIES)
    message(STATUS "Found regex: ${regex_LIBRARIES}")
  else()
    message(STATUS "Found regex")
  endif()
endif()
