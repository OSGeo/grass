#include<stdio.h>
#include<math.h>
#include<stdlib.h>

    /* Green Vegetation Index */ 
double g_vi(double bluechan, double greenchan, double redchan,
	      double nirchan, double chan5chan, double chan7chan) 
{
    double result;

    if ((nirchan + redchan) == 0.0) {
	result = -1.0;
    }
    else
	 {
	result =
	    (-0.2848 * bluechan - 0.2435 * greenchan - 0.5436 * redchan +
	     0.7243 * nirchan + 0.0840 * chan5chan - 0.1800 * chan7chan);
	}
    return result;
}


