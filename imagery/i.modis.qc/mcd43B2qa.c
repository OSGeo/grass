/* Band-wise Albedo Quality Data 1Km long Int
 * SDS: BRDF_Albedo_Band_Quality
 * bits[0-3][4-7][8-11][12-15][16-19][20-23][24-27]
 * 0000 -> class 0: best quality, 75% or more with best full inversions 
 * 0001 -> class 1: good quality, 75% or more with full inversions
 * 0010 -> class 2: Mixed, 50% or less full inversions and 25% or less fill values 
 * 0011 -> class 3: All magnitude inversions or 50% or less fill values 
 * 0100 -> class 4: 75% or more fill values 
 * Classes 5-14: Not Used
 * 1111 -> class 15: Fill Value
 */ 

#include <grass/raster.h>

CELL mcd43B2qa(CELL pixel, int bandno) 
{
    CELL qctemp;

    pixel >>= 4 * (bandno - 1);	/* bitshift [] to [0-3] etc. */
    qctemp = pixel & 0x0F;    
    
    return qctemp;
}


