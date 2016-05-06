/* mod13Q1 Possible Snow/Ice 250m bits[14]
 * 0 -> class 0: No
 * 1 -> class 1: Yes
 */  

#include <grass/raster.h>

CELL mod13Q1h(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 14;		/*bit [14] becomes [0] */
    qctemp = pixel & 0x01;
    
    return qctemp;
}


