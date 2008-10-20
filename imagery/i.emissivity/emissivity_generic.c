#include<stdio.h>
#include<math.h>
#include<stdlib.h>

    /* Emissivity Generic mode (Reads directly from NDVI)
     * Estimation in the 8-14 micrometers range for sparse canopy
     */ 
double emissivity_generic(double ndvi) 
{
    double result;

    if (ndvi < 0.16) 
	result = 1.0;
    else if (ndvi > 0.74) 
	result = 0.9;
    else 
	result = 1.009 + 0.047 * log(ndvi);
    return result;
}


