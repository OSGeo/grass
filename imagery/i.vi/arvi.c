#include<stdio.h>
#include<math.h>
#include<stdlib.h>
 
    /*  ARVI is resistant to atmospheric effects (in comparison to the NDVI) and 
       is accomplished by a self correcting process for the atmospheric effect in the
       red channel, using the difference in the radiance between the blue and the red channels. 
       (Kaufman and Tanre 1996) */ 
    
    /* Atmospheric Resistant Vegetation Index */ 
double ar_vi(double redchan, double nirchan, double bluechan) 
{
    double result;

    if ((nirchan + redchan) == 0.0) {
	result = -1.0;
    }
    else {
	result =
	    (nirchan - (2 * redchan - bluechan)) / (nirchan +
						    (2 * redchan - bluechan));
    }
    return result;
}


