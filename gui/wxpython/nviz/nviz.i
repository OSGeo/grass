/* File: nviz.i */

%module grass7_wxnviz
%{
#include <grass/gsurf.h>
#include <grass/gstypes.h>
#include "nviz.h"
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

%include "nviz.h"
