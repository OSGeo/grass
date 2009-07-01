/* AEROSOL QUANTITY unsigned int bits[6-7]
 * 00 -> class 0: Climatology
 * 01 -> class 1: Low
 * 10 -> class 2: Average
 * 11 -> class 3: High
 */  

#include <grass/raster.h>

CELL mod09A1sd(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 6;
    qctemp = pixel & 0x03;

    return qctemp;
}


