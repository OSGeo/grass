#include<stdio.h>
#include<math.h>
#include<stdlib.h>

    /* Weighted Difference Vegetation Index */ 
double wd_vi(double redchan, double nirchan) 
{
    double result;

    double a = 1;		/*slope of soil line */

    if ((nirchan + redchan) == 0.0) {
	result = -1.0;
    }
    else {
	result = (nirchan - a * redchan);
    }
    return result;
}


