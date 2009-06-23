#include<stdio.h>
#include<math.h>
#include<stdlib.h>
 double biomass(double fpar, double solar_day, double evap_fr,
		 double light_use_ef) 
{
    double result, apar, conversion_coefs;

    
	/*fPAR in Bastiaassen and Ali = a (= 1.257) * NDVI + b (= -0.161); */ 
	/*light use efficiency is variable for each crop! */ 
	/* Cotton light_use_ef in Uzbekistan is 1.9 */ 
	
	/*      apar                    = (b+a*ndvi); */ 
	conversion_coefs = 0.0864 * 10;
    apar = fpar * (0.48 * solar_day);
    result = apar * light_use_ef * evap_fr * conversion_coefs;
    return result;
}


