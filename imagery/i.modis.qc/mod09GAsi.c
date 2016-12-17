/* Pixel adjacent to cloud unsigned int bits[13]
 * 0 -> class 0: No
 * 1 -> class 1: Yes
 */  

#include <grass/raster.h>

CELL mod09GAsi(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 13;
    qctemp = pixel & 0x01;

    return qctemp;
}


