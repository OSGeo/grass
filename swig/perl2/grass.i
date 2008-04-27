%module Grass
%{
#include "grass/gis.h"
%}
%include "grass/gisdefs.h"

%include typemaps.i

void set_my_error_routine();
