#include <stdio.h>
#include <math.h>
#include <stdlib.h>

/*
   PVI: Perpendicular Vegetation Index

   PVI = sin(a)NIR-cos(a)red
   for a  isovegetation lines (lines of equal vegetation) would all be parallel
   to the soil line therefore a=1

 */
<<<<<<< HEAD
double p_vi(double redchan, double nirchan, double soil_line_slope)
=======
double p_vi(double redchan, double nirchan)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
{
    double result, a;

    a = soil_line_slope;

    if ((nirchan + redchan) == 0.0) {
        result = -1.0;
    }
    else {
<<<<<<< HEAD
        result = (sin(a) * nirchan) - (cos(a) * redchan);
=======
        result = (sin(1) * nirchan) - (cos(1) * redchan);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
    }
    return result;
}
