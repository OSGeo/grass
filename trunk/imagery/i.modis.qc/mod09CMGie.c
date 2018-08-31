/* snow unsigned int bits[2]
 * 0 -> class 0: no
 * 1 -> class 1: yes
 */  

#include <grass/raster.h>

CELL mod09CMGie(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 4;
    qctemp = pixel & 0x01;

    return qctemp;
}


