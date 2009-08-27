%{
#include <stdio.h>
#include <grass/gis.h>
#include <grass/raster.h>

#if PY_VERSION_HEX < 0x02050000 && !defined(PY_SSIZE_T_MIN)
typedef int Py_ssize_t;
# define PY_SSIZE_T_MAX INT_MAX
# define PY_SSIZE_T_MIN INT_MIN
#endif

%}

%rename(my_def) def;
//%rename(my_class) class;

%include "file.i"

%include "my_typemaps.i"
