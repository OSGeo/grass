/* sun glint unsigned int bits[2]
 * 0 -> class 0: no
 * 1 -> class 1: yes
 */  

#include <grass/raster.h>

CELL mod09CMGig(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 6;
    qctemp = pixel & 0x01;

    return qctemp;
}


