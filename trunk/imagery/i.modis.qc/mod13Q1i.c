/* mod13Q1 Possible Shadow 250m bits[15]
 * 0 -> class 0: No
 * 1 -> class 1: Yes
 */  

#include <grass/raster.h>

CELL mod13Q1i (CELL pixel) 
{
    CELL qctemp;

    pixel >>= 15;		/*bit [15] becomes [0] */
    qctemp = pixel & 0x01;
    
    return qctemp;
}


