/* Different orbit from 500m product, 250m Unsigned Int bit[14]
 * 0 -> class 0: same orbit as 500m
 * 1 -> class 1: different orbit from 500m
 */  

#include <grass/raster.h>

CELL mod09Q1f(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 14;		/* bit no 14 becomes 0 */
    qctemp = pixel & 0x01;
    
    return qctemp;
}


