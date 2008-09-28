#include<stdio.h>
#include<math.h>
#include<stdlib.h>

    /* MSAVI: Modified Soil Adjusted Vegetation Index       
     *                                                      
     *                       s(NIR-s*red-a)                 
     *                MSAVI = ---------------------------   
     *                        (a*NIR+red-a*s+X*(1+s*s))     
     *      where a is the soil line intercept, s is the    
     *      soil line slope, and X  is an adjustment factor 
     *      which is set to minimize soil noise (0.08 in    
     *      original papers).                               
     */ 
double msa_vi(double redchan, double nirchan) 
{
    double result;

    if ((nirchan + redchan) == 0.0) {
	result = -1.0;
    }
    else {
	result =
	    (1 / 2) * (2 * (nirchan + 1) -
		       sqrt((2 * nirchan + 1) * (2 * nirchan + 1)) -
		       (8 * (nirchan - redchan)));
    }
    return result;
}


