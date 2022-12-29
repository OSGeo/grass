/* MODLAND QA Bits 250m Unsigned Int bits[0-1]
 * 00 -> class 0: Corrected product produced at ideal quality -- all bands
 * 01 -> class 1: Corrected product produced at less than idel quality -- some or all bands
 * 10 -> class 2: Corrected product NOT produced due to cloud effect -- all bands
 * 11 -> class 3: Corrected product NOT produced due to other reasons -- some or all bands mayb be fill value (Note that a value of [11] overrides a value of [01])
 */  
#include <grass/raster.h>

CELL mod09Q1a (CELL pixel) 
{
    CELL qctemp;
    qctemp = pixel & 0x03;
    
    return qctemp;
}


