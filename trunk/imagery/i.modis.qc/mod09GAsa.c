/* Cloud State unsigned int bits[0-1]
 * 00 -> class 0: clear
 * 01 -> class 1: cloudy
 * 10 -> class 2: mixed
 * 11 -> class 3: not set, assumed clear
 */  

#include <grass/raster.h>

CELL mod09GAsa(CELL pixel) 
{
    CELL qctemp;

    /* Select bit 0 and 1 (right-side).
     * hexadecimal "0x03" => binary "11" 
     * this will set all other bits to null */
    qctemp = pixel & 0x03;

    return qctemp;
}


