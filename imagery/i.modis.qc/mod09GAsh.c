/* MOD35 snow/ice flag unsigned int bits [12]
 * 0 -> class 0: No
 * 1 -> class 1: Yes
 */  

#include <grass/raster.h>

CELL mod09GAsh(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 12;
    qctemp = pixel & 0x01;

    return qctemp;
}


