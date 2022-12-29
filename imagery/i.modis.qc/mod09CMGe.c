/* Adjacency correction 500m long Int bit[31]
 * 0 -> class 0: Not Corrected product
 * 1 -> class 1: Corrected product
 */  

#include <grass/raster.h>

CELL mod09CMGe(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 31;		/* bit no 31 becomes 0 */
    qctemp = pixel & 0x01; 
    
    return qctemp;
}


