#include<stdio.h>
#include<math.h>
#include<stdlib.h>

    /* Normalized Difference Vegetation Index */ 
double nd_vi(double redchan, double nirchan) 
{
    double result;

    if ((nirchan + redchan) == 0.0) {
	result = -1.0;
    }
    else {
	result = (nirchan - redchan) / (nirchan + redchan);
    }
    return result;
}


