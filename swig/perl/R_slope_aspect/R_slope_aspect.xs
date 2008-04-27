#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"

MODULE = R_slope_aspect		PACKAGE = R_slope_aspect		

int 
r_slope_aspect (argc, argv);
	int argc
	char **argv
