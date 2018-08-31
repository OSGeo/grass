/* mod13Q1 Adjacent cloud detected 250m bit[8]
 * 00 -> class 0: No
 * 01 -> class 1: Yes
 */  

#include <grass/raster.h>

CELL mod13Q1d (CELL pixel) 
{
    CELL qctemp;

    pixel >>= 8;		/*bit [8] becomes [0] */
    qctemp = pixel & 0x01;
    
    return qctemp;
}


