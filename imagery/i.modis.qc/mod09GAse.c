/* CIRRUS DETECTED unsigned int bits[8-9]
 * 00 -> class 0: None
 * 01 -> class 1: Small
 * 10 -> class 2: Average
 * 11 -> class 3: High
 */  

#include <grass/raster.h>

CELL mod09GAse(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 8;
    qctemp = pixel & 0x03;

    return qctemp;
}


