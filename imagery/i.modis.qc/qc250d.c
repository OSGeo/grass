/* Atmospheric correction 250m Unsigned Int bit[12]
 * 0 -> class 0: Not Corrected product
 * 1 -> class 1: Corrected product
 */  

#include "grass/gis.h"

CELL qc250d(CELL pixel) 
{
    CELL qctemp;

    pixel >>= 12;		/* bit no 12 becomes 0 */
    qctemp = pixel & 0x01;
    
    return qctemp;
}

