# AUTHOR(S): Rashad Kanavath <rashad km gmail>
# PURPOSE: 	 CMake macro to build a grass executable (program) under sub directory
#            which is passed as argument to macro
# COPYRIGHT: (C) 2000 by the GRASS Development Team
#   	    	 This program is free software under the GPL (>=v2)
#   	    	 Read the file COPYING that comes with GRASS for details.

macro(build_program_in_subdir dir_name)
  set (extra_args ${ARGN})
  if ("NAME" IN_LIST extra_args)
    #message("dir_name=${dir_name} ${extra_args}")
    build_program(SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/${dir_name} ${ARGN} )
  else()
    get_filename_component(pgm_name ${dir_name} NAME)
    build_program(NAME ${pgm_name} SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/${dir_name} ${ARGN} )
  endif()
endmacro()
