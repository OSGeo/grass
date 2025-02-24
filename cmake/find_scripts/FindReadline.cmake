find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(Readline QUIET "readline")
  pkg_check_modules(History QUIET "history")
  set(Readline_LIBRARIES ${Readline_LINK_LIBRARIES})
  set(History_LIBRARIES ${History_LINK_LIBRARIES})
else()
  find_path(Readline_ROOT_DIR NAMES include/readline/readline.h)
  find_path(
    Readline_INCLUDE_DIRS
    NAMES readline/readline.h
    HINTS ${Readline_ROOT_DIR}/include)
  find_library(
    Readline_LIBRARIES
    NAMES readline
    HINTS ${Readline_ROOT_DIR}/lib)
  if(Readline_INCLUDE_DIRS AND Readline_LIBRARIES)
    set(Readline_FOUND TRUE)
  endif()

  find_path(
    History_INCLUDE_DIRS
    NAMES readline/readline.h
    HINTS ${Readline_ROOT_DIR}/include)
  find_library(
    History_LIBRARIES
    NAMES readline
    HINTS ${Readline_ROOT_DIR}/lib)
  if(History_INCLUDE_DIRS AND History_LIBRARIES)
    set(History_FOUND TRUE)
  endif()
endif()

if(Readline_FOUND)
  message(STATUS "Found Readline: ${Readline_LIBRARIES}")
endif()

if(History_FOUND)
  message(STATUS "Found History: ${History_LIBRARIES}")
endif()
