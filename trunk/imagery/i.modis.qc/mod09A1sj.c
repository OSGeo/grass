/* BRDF correction performed unsigned int bits[14]
 * 0 -> class 0: No
 * 1 -> class 1: Yes
 */  

#include <grass/raster.h>

CELL mod09A1sj(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 14;
    qctemp = pixel & 0x01;

    return qctemp;
}


