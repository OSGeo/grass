/*  AOT (aerosol optical thinkness) has climatological values unsigned int bits[2]
 * 0 -> class 0: no
 * 1 -> class 1: yes
 */  

#include <grass/raster.h>

CELL mod09CMGin(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 14;
    qctemp = pixel & 0x01;

    return qctemp;
}


