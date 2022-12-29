#include <stdio.h>

#include <grass/gis.h>
#include <grass/gprojects.h>
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

void list_bands(struct Cell_head *cellhd, GDALDatasetH hDS)
{
    struct Cell_head loc_wind;
    struct Key_Value *proj_info = NULL, *proj_units = NULL;
    struct Key_Value *loc_proj_info = NULL, *loc_proj_units = NULL;
    int n_bands, i_band, proj_same;
    GDALRasterBandH hBand;
    GDALDataType gdal_type;

    if (GPJ_wkt_to_grass(cellhd, &proj_info,
                         &proj_units, GDALGetProjectionRef(hDS), 0) < 0) {
        proj_same = 0;
    }
    else {

        G_get_default_window(&loc_wind);
        if (loc_wind.proj != PROJECTION_XY) {
            loc_proj_info = G_get_projinfo();
            loc_proj_units = G_get_projunits();
        }


        if (loc_wind.proj != cellhd->proj ||
            (G_compare_projections
             (loc_proj_info, loc_proj_units, proj_info, proj_units)) < 0) {
            proj_same = 0;
        }
        else {
            proj_same = 1;
        }

    }

    n_bands = GDALGetRasterCount(hDS);

    for (i_band = 1; i_band <= n_bands; i_band++) {

        hBand = GDALGetRasterBand(hDS, i_band);
        gdal_type = GDALGetRasterDataType(hBand);

        fprintf(stdout, "%d,%s,%d\n", i_band, GDALGetDataTypeName(gdal_type),
                proj_same);

    }
}

