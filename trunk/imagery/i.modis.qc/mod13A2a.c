/* mod13A2 Mandatory QA Flags 1Km bits[0-1]
 * 00 -> class 0: VI produced, good quality
 * 01 -> class 1: VI produced, but check other QA
 * 10 -> class 2: Pixel produced, but most probably cloud
 * 11 -> class 3: Pixel not produced due to other reasons than clouds
 */  

#include <grass/raster.h>

CELL mod13A2a (CELL pixel) 
{
    CELL qctemp;
    qctemp = pixel & 0x03;
    
    return qctemp;
}


