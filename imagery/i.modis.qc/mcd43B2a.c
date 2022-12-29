/* mcd43B2 SDS Albedo Ancillary QA Flags 1Km bits[0-3]
 * 0000 -> class 0: Satellite Platform: Terra
 * 0001 -> class 1: Satellite Platform: Terrra/Aqua
 * 0010 -> class 2: Satellite Platform: Aqua
 * 1111 -> class 15: Fill Value
 * Classes 3-14: Not used
 */  

#include <grass/raster.h>

CELL mcd43B2a (CELL pixel) 
{
    CELL qctemp;

    qctemp = pixel & 0x0F;
    
    return qctemp;
}


