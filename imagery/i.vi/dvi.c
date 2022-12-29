#include<stdio.h>
#include<math.h>
#include<stdlib.h>

    /* DVI: Difference Vegetation Index */ 
double d_vi(double redchan, double nirchan) 
{
    double result;

    if ((nirchan + redchan) == 0.0) {
	result = -1.0;
    }
    else {
	result = (nirchan - redchan);
    }
    return result;
}


