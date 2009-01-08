%{
#include <stdio.h>
#include <grass/gis.h>
%}

%rename(my_def) def;
//%rename(my_class) class;

%include "file.i"

%include "my_typemaps.i"
