#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <grass/gis.h>

    /*
     * Average Solar Diurnal Radiation after Bastiaanssen (1995) 
     */
    
#define PI M_PI

double solar_day(double lat, double doy, double tsw) 
{
    double ws, cosun, n10_temp, delta, ds, result;

    ds = 1.0 + 0.01672 * sin(2 * PI * (doy - 93.5) / 365.0);
    delta = 0.4093 * sin((2 * PI * doy / 365) - 1.39);
    n10_temp = lat * PI / 180.0;
    ws = acos(-tan(n10_temp) * tan(delta * PI / 180.0));
    cosun =
	ws * sin(delta * PI / 180.0) * sin(n10_temp) +
	cos(delta * PI / 180.0) * cos(n10_temp) * sin(ws);
    result = (cosun * 1367 * tsw) / (PI * ds * ds);
    return result;
}


