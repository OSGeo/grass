#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <grass/gis.h>

#define PI M_PI

double r_net(double bbalb, double ndvi, double tempk, double dtair,
	      double e0, double tsw, double doy, double utc,
	      double sunzangle) 
{
    
    /* Tsw =  atmospheric transmissivity single-way (~0.7 -) */ 
    /* DOY = Day of Year */ 
    /* utc = UTC time of sat overpass */ 
    /* sunzangle = sun zenith angle at sat. overpass */ 
    /* tair = air temperature (approximative, or met.station) */ 
    double Kin = 0.0, Lin = 0.0, Lout = 0.0, Lcorr = 0.0, result = 0.0;
    double temp = 0.0, ds = 0.0, e_atm = 0.0, delta = 0.0;
    double tsw_for_e_atm = 0.7;	/*Special tsw, consider it a coefficient */
    
    /* Atmospheric emissivity (Bastiaanssen, 1995) */ 
    e_atm = 1.08 * pow(-log(tsw), 0.265);
    
    /* Atmospheric emissivity (Pawan, 2004) */ 
    /*      e_atm   = 0.85 * pow(-log(tsw),0.09); */ 
	
    /*      ds = 1.0 + 0.01672 * sin(2*PI*(doy-93.5)/365); */ 
    ds = 1.0 / pow((1 + 0.033 * cos(2 * PI * doy / 365)), 2);
    delta = 0.4093 * sin((2 * PI * doy / 365) - 1.39);
    
    /* Kin is the shortwave incoming radiation */ 
    Kin = 1358.0 * (cos(sunzangle * PI / 180) * tsw / (ds * ds));
    
    /* Lin is incoming longwave radiation */ 
    Lin = (e_atm) * 5.67 * pow(10, -8) * pow((tempk - dtair), 4);
    
    /* Lout is surface grey body emission in Longwave spectrum */ 
    Lout = e0 * 5.67 * pow(10, -8) * pow(tempk, 4);
    
    /* Lcorr is outgoing longwave radiation "reflected" by the emissivity */ 
    Lcorr = (1.0 - e0) * Lin;

    result = (1.0 - bbalb) * Kin + Lin - Lout - Lcorr;
    return result;
}


