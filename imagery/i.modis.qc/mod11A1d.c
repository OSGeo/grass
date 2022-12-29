/* mod11A1 LST Error Flag bits[6-7]
 * 00 -> class 0: Average LST error <= 1
 * 01 -> class 1: Average LST error <= 2K
 * 10 -> class 2: Average LST error <= 3K
 * 11 -> class 3: Average LST error > 3K
 */  

#include <grass/raster.h>

CELL mod11A1d(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 6;		/*bits [6-7] become [0-1] */
    qctemp = pixel & 0x03;
    
    return qctemp;
}


