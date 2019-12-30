# AUTHOR(S): Rashad Kanavath <rashad km gmail>
# PURPOSE: 	 CMake macro to build a grass library under sub directory 
#            which is passed as argument to macro
# COPYRIGHT: (C) 2000 by the GRASS Development Team
#   	    	 This program is free software under the GPL (>=v2)
#   	    	 Read the file COPYING that comes with GRASS for details.

macro(build_library_in_subdir dir_name)
  set (extra_args ${ARGN})
  if ("NAME" IN_LIST extra_args)
    build_module(SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/${dir_name} ${ARGN} )
  else()
    get_filename_component(g_name ${dir_name} NAME)
    #message("dir_name=${dir_name} |g_name= ${g_name}")
    build_module(NAME grass_${g_name} SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/${dir_name} ${ARGN})
  endif()
endmacro()