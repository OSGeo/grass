%module Grass
%{
#include "gis.h"
%}
%include "gisdefs.h"

%include typemaps.i

int r_slope_aspect (int argc, char **argv);


