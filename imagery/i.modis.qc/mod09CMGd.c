/* Atmospheric correction 500m long Int bit[30]
 * 0 -> class 0: Not Corrected product
 * 1 -> class 1: Corrected product
 */  

#include <grass/raster.h>

CELL mod09CMGd(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 30;		/* bit no 30 becomes 0 */
    qctemp = pixel & 0x01;    

    return qctemp;
}


