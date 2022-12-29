/* mod13A2 Possible Shadow 1Km bits[15]
 * 0 -> class 0: No
 * 1 -> class 1: Yes
 */  

#include <grass/raster.h>

CELL mod13A2i (CELL pixel) 
{
    CELL qctemp;

    pixel >>= 15;		/*bit [15] becomes [0] */
    qctemp = pixel & 0x01;
    
    return qctemp;
}


