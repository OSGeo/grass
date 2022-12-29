/* criteria used for aerosol retrieval unsigned int bits[2]
 * 0 -> class 0: criterion 1
 * 1 -> class 1: criterion 2
 */  

#include <grass/raster.h>

CELL mod09CMGim(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 13;
    qctemp = pixel & 0x01;

    return qctemp;
}


