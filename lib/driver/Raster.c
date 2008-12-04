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

void COM_begin_raster(int mask, int src[2][2], double dst[2][2])
{
    if (driver->Begin_raster)
	(*driver->Begin_raster) (mask, src, dst);
}

int COM_raster(int n, int row,
	       const unsigned char *red, const unsigned char *grn,
	       const unsigned char *blu, const unsigned char *nul)
{
    if (driver->Raster)
	return (*driver->Raster) (n, row, red, grn, blu, nul);

    return -1;
}

void COM_end_raster(void)
{
    if (driver->End_raster)
	(*driver->End_raster) ();
}
