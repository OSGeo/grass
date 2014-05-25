#include <stdio.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include <gdal.h>

#include "proto.h"

void list_formats(void)
{
    /* -------------------------------------------------------------------- */
    /*      List supported formats and exit.                                */
    /*         code from GDAL 1.2.5  gcore/gdal_misc.cpp                    */
    /*         Copyright (c) 1999, Frank Warmerdam                          */
    /* -------------------------------------------------------------------- */
    int iDr;

    G_message(_("Supported formats:"));
    for (iDr = 0; iDr < GDALGetDriverCount(); iDr++) {
	GDALDriverH hDriver = GDALGetDriver(iDr);
	const char *pszRWFlag;

#ifdef GDAL_DCAP_RASTER
            /* Starting with GDAL 2.0, vector drivers can also be returned */
            /* Only keep raster drivers */
            if (!GDALGetMetadataItem(hDriver, GDAL_DCAP_RASTER, NULL))
                continue;
#endif

	if (GDALGetMetadataItem(hDriver, GDAL_DCAP_CREATE, NULL))
	    pszRWFlag = "rw+";
	else if (GDALGetMetadataItem(hDriver, GDAL_DCAP_CREATECOPY, NULL))
	    pszRWFlag = "rw";
	else
	    pszRWFlag = "ro";

	fprintf(stdout, " %s (%s): %s\n",
		GDALGetDriverShortName(hDriver),
		pszRWFlag, GDALGetDriverLongName(hDriver));
    }
}
