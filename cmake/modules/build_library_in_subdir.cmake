#[[
AUTHOR(S):  Rashad Kanavath <rashad km gmail>
PURPOSE:    This is the macro to make grass libraries through build_module function.
COPYRIGHT:  (C) 2020 by the GRASS Development Team

SPDX-License-Identifier: GPL-2.0-or-later
#]]

macro(build_library_in_subdir dir_name)
  set(extra_args ${ARGN})
  if("NAME" IN_LIST extra_args)
    build_module(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${dir_name} ${ARGN})
  else()
    get_filename_component(g_name ${dir_name} NAME)
    # message("dir_name=${dir_name} |g_name= ${g_name}")
    build_module(NAME grass_${g_name} SRC_DIR
                 ${CMAKE_CURRENT_SOURCE_DIR}/${dir_name} ${ARGN})
  endif()
endmacro()
