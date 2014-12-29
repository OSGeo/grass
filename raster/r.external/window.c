#include <grass/gis.h>
#include <grass/glocale.h>

#include <gdal.h>

#include "proto.h"

void setup_window(struct Cell_head *cellhd, GDALDatasetH hDS, int *flip)
{
    /* -------------------------------------------------------------------- */
    /*      Set up the window representing the data we have.                */
    /* -------------------------------------------------------------------- */

    double adfGeoTransform[6];

    cellhd->rows = GDALGetRasterYSize(hDS);
    cellhd->rows3 = GDALGetRasterYSize(hDS);
    cellhd->cols = GDALGetRasterXSize(hDS);
    cellhd->cols3 = GDALGetRasterXSize(hDS);

    if (GDALGetGeoTransform(hDS, adfGeoTransform) == CE_None) {
	if (adfGeoTransform[2] != 0.0 || adfGeoTransform[4] != 0.0)
	    G_fatal_error(_("Input raster map is rotated - cannot import. "
			    "You may use 'gdalwarp' to transform the map to North-up."));
	if (adfGeoTransform[1] <= 0.0) {
	    G_message(_("Applying horizontal flip"));
	    *flip |= FLIP_H;
	}
	if (adfGeoTransform[5] >= 0.0) {
	    G_message(_("Applying vertical flip"));
	    *flip |= FLIP_V;
	}

	cellhd->north = adfGeoTransform[3];
	cellhd->ns_res = fabs(adfGeoTransform[5]);
	cellhd->ns_res3 = fabs(adfGeoTransform[5]);
	cellhd->south = cellhd->north - cellhd->ns_res * cellhd->rows;
	cellhd->west = adfGeoTransform[0];
	cellhd->ew_res = fabs(adfGeoTransform[1]);
	cellhd->ew_res3 = fabs(adfGeoTransform[1]);
	cellhd->east = cellhd->west + cellhd->cols * cellhd->ew_res;
	cellhd->top = 1.;
	cellhd->bottom = 0.;
	cellhd->tb_res = 1.;
	cellhd->depths = 1;
    }
    else {
	cellhd->north = cellhd->rows;
	cellhd->south = 0.0;
	cellhd->ns_res = 1.0;
	cellhd->ns_res3 = 1.0;
	cellhd->west = 0.0;
	cellhd->east = cellhd->cols;
	cellhd->ew_res = 1.0;
	cellhd->ew_res3 = 1.0;
	cellhd->top = 1.;
	cellhd->bottom = 0.;
	cellhd->tb_res = 1.;
	cellhd->depths = 1;
    }
}

void update_default_window(struct Cell_head *cellhd)
{
    /* -------------------------------------------------------------------- */
    /*      Extend current window based on dataset.                         */
    /* -------------------------------------------------------------------- */

    struct Cell_head cur_wind;

    if (strcmp(G_mapset(), "PERMANENT") == 0) 
	/* fixme: expand WIND and DEFAULT_WIND independently. (currently
	 WIND gets forgotten and DEFAULT_WIND is expanded for both) */
	G_get_default_window(&cur_wind);
    else
	G_get_window(&cur_wind);

    cur_wind.north = MAX(cur_wind.north, cellhd->north);
    cur_wind.south = MIN(cur_wind.south, cellhd->south);
    cur_wind.west = MIN(cur_wind.west, cellhd->west);
    cur_wind.east = MAX(cur_wind.east, cellhd->east);

    cur_wind.rows = (int)ceil((cur_wind.north - cur_wind.south)
			      / cur_wind.ns_res);
    cur_wind.south = cur_wind.north - cur_wind.rows * cur_wind.ns_res;

    cur_wind.cols = (int)ceil((cur_wind.east - cur_wind.west)
			      / cur_wind.ew_res);
    cur_wind.east = cur_wind.west + cur_wind.cols * cur_wind.ew_res;

    if (strcmp(G_mapset(), "PERMANENT") == 0) {
	G_put_element_window(&cur_wind, "", "DEFAULT_WIND");
	G_message(_("Default region for this location updated")); 
    }
    G_put_window(&cur_wind);
    G_message(_("Region for the current mapset updated"));
}
