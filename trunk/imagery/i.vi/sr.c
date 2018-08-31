#include<stdio.h>
#include<math.h>
#include<stdlib.h>

    /* Simple Vegetation ratio */ 
double s_r(double redchan, double nirchan) 
{
    double result;

    if ((redchan) == 0.0) {
	result = -1.0;
    }
    else {
	result = (nirchan / redchan);
    }
    return result;
}


