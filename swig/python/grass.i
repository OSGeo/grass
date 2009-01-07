%module grass

%include "common.i"

%include "carrays.i"
%array_functions(int, intArray);
%array_functions(float, floatArray);
%array_functions(double, doubleArray);

%include "cpointer.i"
%pointer_functions(int, intp);
%pointer_functions(float, floatp);
%pointer_functions(double, doublep);

%include "grass/gis.h"
%include "grass/gisdefs.h"

%pythoncode %{
def G_gisinit(pgm):
    G__gisinit(GIS_H_VERSION, pgm)

%}
