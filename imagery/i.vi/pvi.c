#include <stdio.h>
#include <math.h>
#include <stdlib.h>

/*
   PVI: Perpendicular Vegetation Index

   PVI = sin(a)NIR-cos(a)red
   for a  isovegetation lines (lines of equal vegetation) would all be parallel
   to the soil line therefore a=1

 */
double p_vi(double redchan, double nirchan, double soil_line_slope)
{
    double result, a;

    a = soil_line_slope;

    if ((nirchan + redchan) == 0.0) {
        result = -1.0;
    }
    else {
        result = (sin(a) * nirchan) - (cos(a) * redchan);
    }
    return result;
}
