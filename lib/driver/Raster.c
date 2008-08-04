#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include "driver.h"
#include "driverlib.h"

/******************************************************************************
 * These routines support the drawing of multi-band images on the graphics
 * device.
 ******************************************************************************
 */

void COM_begin_scaled_raster(int mask, int src[2][2], int dst[2][2])
{
    if (driver->Begin_scaled_raster)
	(*driver->Begin_scaled_raster) (mask, src, dst);
}

int COM_scaled_raster(int n, int row,
		      const unsigned char *red, const unsigned char *grn,
		      const unsigned char *blu, const unsigned char *nul)
{
    if (driver->Scaled_raster)
	return (*driver->Scaled_raster) (n, row, red, grn, blu, nul);

    return -1;
}

void COM_end_scaled_raster(void)
{
    if (driver->End_scaled_raster)
	(*driver->End_scaled_raster) ();
}
