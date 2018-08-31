/* Band-wise Data Quality 500m long Int 
 * bits[2-5][6-9][10-13][14-17][18-21][22-25][26-29]
 * 0000 -> class 0: highest quality
 * 0111 -> class 1: noisy detector
 * 1000 -> class 2: dead detector; data interpolated in L1B
 * 1001 -> class 3: solar zenith >= 86 degrees
 * 1010 -> class 4: solar zenith >= 85 and < 86 degrees
 * 1011 -> class 5: missing input
 * 1100 -> class 6: internal constant used in place of climatological data for at least one atmospheric constant
 * 1101 -> class 7: correction out of bounds, pixel constrained to extreme allowable value
 * 1110 -> class 8: L1B data faulty
 * 1111 -> class 9: not processed due to deep ocean or cloud
 * Class 10-15: Combination of bits unused
 */  

#include <grass/raster.h>

CELL mod09A1c(CELL pixel, int bandno) 
{
    CELL qctemp;

    pixel >>= 2 + (4 * (bandno - 1));	/* bitshift [] to [0-3] etc. */
    qctemp = pixel & 0x0F;    
    
    return qctemp;
}


