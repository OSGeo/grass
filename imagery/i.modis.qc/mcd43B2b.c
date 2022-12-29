/* mcd43B2 SDS Albedo Ancillary QA Land/Water Flags 1Km bits[4-7]
 * 0000 -> class 0: Shallow Ocean
 * 0001 -> class 1: Land (Nothing else but land)
 * 0010 -> class 2: Ocean and lake shorelines
 * 0011 -> class 3: Shallow inland water
 * 0100 -> class 4: Ephemeral water
 * 0101 -> class 5: Deep inland water
 * 0110 -> class 6: Moderate or continental ocean
 * 0111 -> class 7: Deep ocean
 * 1111 -> class 15: Fill Value
 * Classes 8-14: Not used
 */  

#include <grass/raster.h>

CELL mcd43B2b (CELL pixel) 
{
    CELL qctemp;

    pixel >>= 4;		/*bits [4-7] become [0-3] */
    qctemp = pixel & 0x0F;
    
    return qctemp;
}


