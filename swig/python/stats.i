%module stats

%include "common.i"

%{
#undef c_sum
#include <grass/stats.h>
%}

%include "grass/stats.h"
