%module vector

%include "common.i"

%{
#include <grass/Vect.h>
#include <grass/vect/dig_structs.h>
%}

%include "grass/Vect.h"
%include "grass/vect/dig_structs.h"
%include "grass/vect/dig_defines.h"
