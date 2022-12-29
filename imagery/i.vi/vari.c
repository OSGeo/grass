#include<stdio.h>
#include<math.h>
#include<stdlib.h>

/*VARI: Visible Atmospherically Resistant Index */ 
double va_ri(double redchan, double bluechan,
	      double greenchan) 
{
/* VARI is the Visible Atmospherically Resistant Index, it was 
 * designed to introduce an atmospheric self-correction 
 * Gitelson A.A., Kaufman Y.J., Stark R., Rundquist D., 2002.
 * Novel algorithms for estimation of vegetation fraction 
 * Remote Sensing of Environment (80), pp76-87.  */
    double result;
    result = (greenchan - redchan ) / (greenchan + 
		redchan - bluechan);
    return result;
}


