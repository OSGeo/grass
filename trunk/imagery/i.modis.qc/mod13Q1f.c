/* mod13Q1 Mixed clouds 250m bit[10]
 * 00 -> class 0: No
 * 01 -> class 1: Yes
 */  

#include <grass/raster.h>

CELL mod13Q1f (CELL pixel) 
{
    CELL qctemp;

    pixel >>= 10;		/*bit [10] becomes [0] */
    qctemp = pixel & 0x01;
    
    return qctemp;
}


