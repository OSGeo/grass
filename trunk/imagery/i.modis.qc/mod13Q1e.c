/* mod13Q1 Atmosphere BRDF correction performed 250m bit[9]
 * 00 -> class 0: No
 * 01 -> class 1: Yes
 */  

#include <grass/raster.h>

CELL mod13Q1e (CELL pixel) 
{
    CELL qctemp;

    pixel >>= 9;		/*bit [9] becomes [0] */
    qctemp = pixel & 0x01;
    
    return qctemp;
}


