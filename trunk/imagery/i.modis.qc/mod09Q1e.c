/* Adjacency correction 250m Unsigned Int bit[13]
 * 0 -> class 0: Not Corrected product
 * 1 -> class 1: Corrected product
 */  

#include <grass/raster.h>

CELL mod09Q1e(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 13;		/* bit no 13 becomes 0 */
    qctemp = pixel & 0x01;
    
    return qctemp;
}


