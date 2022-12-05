# AUTHOR(S): Rashad Kanavath <rashad km gmail>
# PURPOSE: 	 Shortcut macro to call build_module with EXE argument set
# COPYRIGHT: (C) 2020 by the GRASS Development Team
#   	    	 This program is free software under the GPL (>=v2)
#   	    	 Read the file COPYING that comes with GRASS for details.
macro(build_program)
  build_module(${ARGN} EXE)
endmacro()
