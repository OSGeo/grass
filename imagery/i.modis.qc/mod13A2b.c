/* mod13A2 VI Usefulness Flag bits[2-5]
 * 0000 -> class 0: Highest quality
 * 0001 -> class 1: Lower quality
 * 0010 -> class 2: Decreasing quality
 * 0100 -> class 3: Decreasing quality
 * 1000 -> class 4: Decreasing quality
 * 1001 -> class 5: Decreasing quality
 * 1010 -> class 6: Decreasing quality
 * 1100 -> class 7: Lowest quality
 * 1101 -> class 8: Quality so low that it is not useful
 * 1110 -> class 9: L1B data faulty
 * 1111 -> class 10: Not useful for any other reason/not processed
 */  

#include <grass/raster.h>

CELL mod13A2b(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 2;		/*bits [2-5] become [0-4] */
    qctemp = pixel & 0x0F;
    
    return qctemp;
}


