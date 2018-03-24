#include<stdio.h>
#include<math.h>
#include<stdlib.h>

    /* MSAVI2: second Modified Soil Adjusted Vegetation Index
     *      MSAVI2 = (1/2)*(2(NIR+1)-sqrt((2*NIR+1)^2-8(NIR-red)))
     */ 
double msa_vi2(double redchan, double nirchan) 
{
    double result;

    if ((nirchan + redchan) == 0.0 || (2*nirchan+1)*(2*nirchan+1) <= 0.0) 
    {
	result = -1.0;
    }
    else 
    {
	result=0.5*(2*nirchan+1-sqrt((2*nirchan+1)*(2*nirchan+1)-8*(nirchan-redchan)));
    }
    return result;
}


