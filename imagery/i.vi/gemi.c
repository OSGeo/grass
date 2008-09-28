#include<stdio.h>
#include<math.h>
#include<stdlib.h>

    /* GEMI: Global Environmental Monitoring Index
     */ 
double ge_mi(double redchan, double nirchan) 
{
    double result;

    if ((nirchan + redchan) == 0.0) {
	result = -1.0;
    }
    else {
	result =
	    (((2 * ((nirchan * nirchan) - (redchan * redchan)) +
	       1.5 * nirchan + 0.5 * redchan) / (nirchan + redchan +
						 0.5)) * (1 -
							  0.25 * (2 *
								  ((nirchan *
								    nirchan) -
								   (redchan *
								    redchan))
								  +
								  1.5 *
								  nirchan +
								  0.5 *
								  redchan) /
							  (nirchan + redchan +
							   0.5))) -
	    ((redchan - 0.125) / (1 - redchan));
    }
    return result;
}


