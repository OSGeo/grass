/* Internal Fire Algorithm Flag unsigned int bits[11]
 * 0 -> class 0: Fire 
 * 1 -> class 1: No fire
 */  

#include <grass/raster.h>

CELL mod09A1sg(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 11;
    qctemp = pixel & 0x01;

    return qctemp;
}


