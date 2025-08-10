/* MODLAND QA Bits 500m long int bits[0-1]
 * 00 -> class 0: Corrected product produced at ideal quality -- all bands
 * 01 -> class 1: Corrected product produced at less than ideal quality -- some
 * or all bands 10 -> class 2: Corrected product NOT produced due to cloud
 * effect -- all bands 11 -> class 3: Corrected product NOT produced due to
 * other reasons -- some or all bands mayb be fill value (Note that a value of
 * [11] overrides a value of [01])
 */

#include <grass/raster.h>

CELL mod09A1a(CELL pixel)
{
    CELL qctemp;

    /* Select bit 0 and 1 (right-side).
     * hexadecimal "0x03" => binary "11"
     * this will set all other bits to null */
    qctemp = pixel & 0x03;

    return qctemp;
}
