/* mod13A2 Land/Water Flags 1Km bits[11-13]
 * 000 -> class 0: Shallow Ocean
 * 001 -> class 1: Land (Nothing else but land)
 * 010 -> class 2: Ocean Coastlines and lake shorelines
 * 011 -> class 3: Shallow inland water
 * 100 -> class 4: Ephemeral water
 * 101 -> class 5: Deep inland water
 * 110 -> class 6: Moderate or continental ocean
 * 111 -> class 7: Deep ocean
 */  

#include <grass/raster.h>

CELL mod13A2g (CELL pixel) 
{
    CELL qctemp;

    pixel >>= 11;		/*bits [11-13] become [0-2] */
    qctemp = pixel & 0x07;
    
    return qctemp;
}


