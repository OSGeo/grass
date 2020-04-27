# AUTHOR(S): Rashad Kanavath <rashad km gmail>
# PURPOSE: 	 Simply copy python file in given subdirectory
#            Destination will be relative to GISBASE directory set in root CMakeLists.txt
# COPYRIGHT: (C) 2020 by the GRASS Development Team
#   	    	 This program is free software under the GPL (>=v2)
#   	    	 Read the file COPYING that comes with GRASS for details.
function(copy_python_files_in_subdir dir_name dst_prefix)
file(GLOB PY_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${dir_name}/*.py")
  string(REPLACE "/" "_" targ_name ${dir_name})
add_custom_target(python_${targ_name}
	COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/${dst_prefix}/${dir_name}/
	COMMAND ${CMAKE_COMMAND} -E copy ${PY_FILES} ${GISBASE}/${dst_prefix}/${dir_name}/
	)
    set_target_properties (python_${targ_name} PROPERTIES FOLDER lib/python)
endfunction()
