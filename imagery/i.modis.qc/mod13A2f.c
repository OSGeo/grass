/* mod13A2 Mixed clouds 1Km bit[10]
 * 00 -> class 0: No
 * 01 -> class 1: Yes
 */  

#include <grass/raster.h>

CELL mod13A2f (CELL pixel) 
{
    CELL qctemp;

    pixel >>= 10;		/*bit [10] becomes [0] */
    qctemp = pixel & 0x01;
    
    return qctemp;
}


