/* dust unsigned int bits[4]
 * 00 -> class 0: no
 * 01 -> class 1: yes
 */  

#include <grass/raster.h>

CELL mod09CMGih(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 7;
    qctemp = pixel & 0x01;

    return qctemp;
}


