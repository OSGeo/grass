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

    struct Cell_head def_wind;

    G_get_default_window(&def_wind);

    def_wind.north = MAX(def_wind.north, cellhd->north);
    def_wind.south = MIN(def_wind.south, cellhd->south);
    def_wind.west = MIN(def_wind.west, cellhd->west);
    def_wind.east = MAX(def_wind.east, cellhd->east);

    def_wind.rows = (int)ceil((def_wind.north - def_wind.south)
			      / def_wind.ns_res);
    def_wind.south = def_wind.north - def_wind.rows * def_wind.ns_res;

    def_wind.cols = (int)ceil((def_wind.east - def_wind.west)
			      / def_wind.ew_res);
    def_wind.east = def_wind.west + def_wind.cols * def_wind.ew_res;

    G__put_window(&def_wind, "../PERMANENT", "DEFAULT_WIND");
}
