/* Internal Cloud Algorithm Flag unsigned int bits[10]
 * 0 -> class 0: No cloud 
 * 1 -> class 1: Cloud
 */  

#include <grass/raster.h>

CELL mod09GAsf(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 10;
    qctemp = pixel & 0x01;

    return qctemp;
}


