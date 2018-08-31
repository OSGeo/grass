/* Cloud State 250m Unsigned Int bits[2-3]
 * 00 -> class 0: Clear -- No clouds
 * 01 -> class 1: Cloudy
 * 10 -> class 2: Mixed
 * 11 -> class 3: Not Set ; Assumed Clear
 */  

#include <grass/raster.h>

CELL mod09Q1b(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 2;		/*bits [2-3] become [0-1] */
    qctemp = pixel & 0x03;
    
    return qctemp;
}


