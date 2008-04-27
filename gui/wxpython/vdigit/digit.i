/* File: digit.i */

%module grass6_wxvdigit
%{
#include <grass/gis.h>
#include <grass/gisdefs.h>
#include <grass/Vect.h>
#include <grass/vect/dig_structs.h>
#include "driver.h"
#include "digit.h"
%}

%include "std_vector.i"
namespace std { 
   %template(IntVector) vector<int>;
   %template(DoubleVector) vector<double>;
}
%include "std_map.i"
namespace std { 
   %template(IntVecIntMap) map<int, vector<int> >;
}
%include "driver.h"
%include "digit.h"