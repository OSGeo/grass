/* mod11A1 Mandatory QA Flags 1Km bits[0-1]
 * 00 -> class 0: LST produced, good quality, not necessary to examine more detailed QA
 * 01 -> class 1: LST produced, other quality, recommend examination of more detailed QA
 * 10 -> class 2: LST not produced due to cloud effects
 * 11 -> class 3: LST not produced primarily due to reasons other than cloud
 */  

#include <grass/raster.h>

CELL mod11A1a (CELL pixel) 
{
    CELL qctemp;
    qctemp = pixel & 0x03;
    
    return qctemp;
}


