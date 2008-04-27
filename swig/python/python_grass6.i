//File : python_grass6.i

%include "carrays.i"
%array_functions(int, intArray);
%array_functions(float, floatArray);
%array_functions(double, doubleArray);

%include "cpointer.i"
%pointer_functions(int, intp);
%pointer_functions(float, floatp);
%pointer_functions(double, doublep);

%module python_grass6
%{
#include <stdio.h>
#include <stdarg.h>
#include <grass/gis.h>
#include <grass/gisdefs.h>
#include <grass/imagery.h>
#include <grass/imagedefs.h>
#include <grass/Vect.h>
#include <grass/vect/dig_structs.h>
%}

%include "my_typemaps.i"
%include "renames.i"
%include "interfaces/gis.i"
%include "interfaces/gisdefs.i"
%include "interfaces/imagery.i"
%include "interfaces/imagedefs.i"
%include "interfaces/vect.i"
%include "interfaces/dig_structs.i"
%include "interfaces/dig_types.i"

