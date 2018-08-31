#include<stdio.h>
#include<math.h>
#include<stdlib.h>

    /*
       IPVI: Infrared Percentage Vegetation Index
       
       NIR
       IPVI = --------
       NIR+red
     */ 
double ip_vi(double redchan, double nirchan) 
{
    double result;

    if ((nirchan + redchan) == 0.0) {
	result = -1.0;
    }
    else {
	result = (nirchan) / (nirchan + redchan);
    }
    return result;
}


