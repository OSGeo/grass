/* Internal Snow Mask unsigned int bits[15]
 * 0 -> class 0: No snow
 * 1 -> class 1: Snow
 */  

#include <grass/raster.h>

CELL mod09GAsk(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 15;
    qctemp = pixel & 0x01;

    return qctemp;
}


