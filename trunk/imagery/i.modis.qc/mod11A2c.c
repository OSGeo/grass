/* mod11A2 Emis Error Flag bits[4-5]
 * 00 -> class 0: Average emissivity error <= 0.01
 * 01 -> class 1: Average emissivity error <= 0.02
 * 10 -> class 2: Average emissivity error <= 0.04
 * 11 -> class 3: Average emissivity error > 0.04
 */  

#include <grass/raster.h>

CELL mod11A2c(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 4;		/*bits [4-5] become [0-1] */
    qctemp = pixel & 0x03;
    
    return qctemp;
}


