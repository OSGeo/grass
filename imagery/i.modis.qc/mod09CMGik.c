/* cirrus unsigned int bits[4]
 * 00 -> class 0: none
 * 01 -> class 1: small
 * 10 -> class 2: average
 * 11 -> class 3: high
 */  

#include <grass/raster.h>

CELL mod09CMGik(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 10;
    qctemp = pixel & 0x03;

    return qctemp;
}


