/* mod13A2 Adjacent cloud detected 1Km bit[8]
 * 00 -> class 0: No
 * 01 -> class 1: Yes
 */  

#include <grass/raster.h>

CELL mod13A2d (CELL pixel) 
{
    CELL qctemp;

    pixel >>= 8;		/*bit [8] becomes [0] */
    qctemp = pixel & 0x01;
    
    return qctemp;
}


