/* mod11A2 Data Quality Flag bits[2-3]
 * 00 -> class 0: Good data quality of L1B in 7 TIR bands
 * 01 -> class 1: Other quality data
 * 10 -> class 2: TBD
 * 11 -> class 3: TBD
 */  

#include <grass/raster.h>

CELL mod11A2b(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 2;		/*bits [2-3] become [0-1] */
    qctemp = pixel & 0x03;
    
    return qctemp;
}


