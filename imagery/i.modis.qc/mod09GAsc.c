/* LAND/WATER FLAG unsigned int bits[3-5]
 * 000 -> class 0: Shallow ocean
 * 001 -> class 1: Land
 * 010 -> class 2: Ocean coastlines and lake shorelines
 * 011 -> class 3: Shallow inland water
 * 100 -> class 4: Ephemeral water
 * 101 -> class 5: Deep inland water
 * 110 -> class 6: Continental/moderate ocean
 * 111 -> class 7: Deep ocean
 */  

#include <grass/raster.h>

CELL mod09GAsc(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 3;
    qctemp = pixel & 0x07;

    return qctemp;
}


