/* Pixel adjacent to cloud unsigned int bits[13]
 * 0 -> class 0: Yes
 * 1 -> class 1: No
 */  

#include <grass/raster.h>

CELL mod09A1si(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 13;
    qctemp = pixel & 0x01;

    return qctemp;
}


