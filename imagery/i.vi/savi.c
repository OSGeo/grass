#include<stdio.h>
#include<math.h>
#include<stdlib.h>

    /* Soil Adjusted Vegetation Index */ 
double sa_vi(double redchan, double nirchan) 
{
    double result;

    if ((nirchan + redchan) == 0.0) {
	result = -1.0;
    }
    else {
	result =
	    ((1 + 0.5) * (nirchan - redchan)) / (nirchan + redchan + 0.5);
    }
    return result;
}


