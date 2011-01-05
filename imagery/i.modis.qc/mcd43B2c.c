/* mcd43B2 SDS Albedo Ancillary QA Sun Zenith Angle at local solar noon bits[8-14]
   Returns integer value [0-90], 127 is Fill Value
 */  

#include <grass/raster.h>

CELL mcd43B2c (CELL pixel) 
{
    CELL qctemp;

    pixel >>= 8;		/*bits [8-14] become [0-7] */
    qctemp = pixel & 0x7F;
    
    return qctemp;
}


