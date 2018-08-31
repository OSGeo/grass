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
double msa_vi(double redchan, double nirchan, double soil_line_slope, double soil_line_intercept, double soil_noise_reduction_factor ) 
{
    double result, a, s, X;
    s = soil_line_slope;
    a = soil_line_intercept;
    X = soil_noise_reduction_factor;

    if ((nirchan + redchan) == 0.0) {
	result = -1.0;
    }
    else {
	result = s*(nirchan-s*redchan-a)/(a*nirchan+redchan-a*s+X*(1+s+s));
    }
    return result;
}


