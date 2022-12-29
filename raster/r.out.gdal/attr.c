#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "cpl_string.h"
#include "gdal.h"
#include "local_proto.h"


int export_attr(GDALDatasetH hMEMDS, int band,
		const char *name, const char *mapset,
		RASTER_MAP_TYPE maptype)
{
    struct Categories cats;
    CELL CellMin;
    CELL CellMax;
    DCELL dfCellMin;
    DCELL dfCellMax;
    char *label;
    struct Colors sGrassColors;
    int rcount;
    int i;
    int ret = 0;
    GDALRasterAttributeTableH hrat;
    GDALRasterBandH hBand;

    Rast_init_cats("Labels", &cats);
    if (Rast_read_cats(name, mapset, &cats))
	return -1;
    
    rcount = 0;
    Rast_init_colors(&sGrassColors);
    if (Rast_read_colors(name, mapset, &sGrassColors) >= 0) {
	rcount = Rast_colors_count(&sGrassColors);
    }

    if (cats.ncats == 0 && rcount == 0)
	return 0;

    /* Get raster band  */
    hBand = GDALGetRasterBand(hMEMDS, band);

    /* Field usage of raster attribute table:
     * GFU_Generic = 0, GFU_PixelCount = 1, GFU_Name = 2,
     * GFU_Min = 3, GFU_Max = 4, GFU_MinMax = 5,
     * GFU_Red = 6, GFU_Green = 7, GFU_Blue = 8, GFU_Alpha = 9,
     * GFU_RedMin = 10, GFU_GreenMin = 11, GFU_BlueMin = 12, GFU_AlphaMin = 13,
     * GFU_RedMax = 14, GFU_GreenMax = 15, GFU_BlueMax = 16, GFU_AlphaMax = 17,
     * GFU_MaxCount
     */

    /* TODO: cats.ncats > 0 && rcount > 0
     * how to merge categories and color rules ?
     * what to do for a cell value that has a category but no color rule ?
     * what to do for a cell value that has a color rule but no category ?
     */
    if (cats.ncats > 0) {
	int use_minmax = 0;

	if (maptype == CELL_TYPE) {
	    for (i = 0; i < cats.ncats; i++) {
		label = Rast_get_ith_c_cat(&cats, i, &CellMin, &CellMax);
		if (CellMin != CellMax) {
		    use_minmax = 1;
		    break;
		}
	    }
	}
	else {
	    for (i = 0; i < cats.ncats; i++) {
		label = Rast_get_ith_d_cat(&cats, i, &dfCellMin, &dfCellMax);
		if (dfCellMin != dfCellMax) {
		    use_minmax = 1;
		    break;
		}
	    }
	}

	/* create new raster attribute table */
	hrat = GDALCreateRasterAttributeTable();
	
	if (use_minmax) {
	    if (maptype == CELL_TYPE) {
		GDALRATCreateColumn(hrat, "min", GFT_Integer, GFU_Min);
		GDALRATCreateColumn(hrat, "max", GFT_Integer, GFU_Max);
	    }
	    else {
		GDALRATCreateColumn(hrat, "min", GFT_Real, GFU_Min);
		GDALRATCreateColumn(hrat, "max", GFT_Real, GFU_Max);
	    }
	    GDALRATCreateColumn(hrat, "label", GFT_String, GFU_Name);

	    GDALRATSetRowCount(hrat, cats.ncats);

	    if (maptype == CELL_TYPE) {
		for (i = 0; i < cats.ncats; i++) {
		    label = Rast_get_ith_c_cat(&cats, i, &CellMin, &CellMax);
		    GDALRATSetValueAsInt(hrat, i, 0, CellMin);
		    GDALRATSetValueAsInt(hrat, i, 1, CellMax);
		    GDALRATSetValueAsString(hrat, i, 2, label);
		}
	    }
	    else {
		for (i = 0; i < cats.ncats; i++) {
		    label = Rast_get_ith_d_cat(&cats, i, &dfCellMin, &dfCellMax);
		    GDALRATSetValueAsDouble(hrat, i, 0, dfCellMin);
		    GDALRATSetValueAsDouble(hrat, i, 1, dfCellMax);
		    GDALRATSetValueAsString(hrat, i, 2, label);
		}
	    }
	}
	else {
	    if (maptype == CELL_TYPE) {
		GDALRATCreateColumn(hrat, "value", GFT_Integer, GFU_MinMax);
	    }
	    else {
		GDALRATCreateColumn(hrat, "value", GFT_Real, GFU_MinMax);
	    }
	    GDALRATCreateColumn(hrat, "label", GFT_String, GFU_Name);

	    GDALRATSetRowCount(hrat, cats.ncats);

	    if (maptype == CELL_TYPE) {
		for (i = 0; i < cats.ncats; i++) {
		    label = Rast_get_ith_c_cat(&cats, i, &CellMin, &CellMax);
		    GDALRATSetValueAsInt(hrat, i, 0, CellMin);
		    GDALRATSetValueAsString(hrat, i, 1, label);
		}
	    }
	    else {
		for (i = 0; i < cats.ncats; i++) {
		    label = Rast_get_ith_d_cat(&cats, i, &dfCellMin, &dfCellMax);
		    GDALRATSetValueAsDouble(hrat, i, 0, dfCellMin);
		    GDALRATSetValueAsString(hrat, i, 1, label);
		}
	    }
	}

	if (GDALSetDefaultRAT(hBand, hrat) != CE_None) {
	    G_warning(_("Failed to set raster attribute table"));
	    ret = -1;
	}
	/* GDALRATDumpReadable(hrat, stdout); */

	GDALDestroyRasterAttributeTable(hrat);
    }
    else if (cats.ncats == 0 && rcount > 0) {
	unsigned char r1, g1, b1, r2, g2, b2;

	/* create new raster attribute table */
	hrat = GDALCreateRasterAttributeTable();
	GDALRATCreateColumn(hrat, "min", GFT_Real, GFU_Min);
	GDALRATCreateColumn(hrat, "max", GFT_Real, GFU_Max);
	GDALRATCreateColumn(hrat, "redmin", GFT_Integer, GFU_RedMin);
	GDALRATCreateColumn(hrat, "redmax", GFT_Integer, GFU_RedMax);
	GDALRATCreateColumn(hrat, "greenmin", GFT_Integer, GFU_GreenMin);
	GDALRATCreateColumn(hrat, "greenmax", GFT_Integer, GFU_GreenMax);
	GDALRATCreateColumn(hrat, "bluemin", GFT_Integer, GFU_BlueMin);
	GDALRATCreateColumn(hrat, "bluemax", GFT_Integer, GFU_BlueMax);

	for (i = 0; i < rcount; i++) {

	    Rast_get_fp_color_rule(&dfCellMin, &r1, &g1, &b1,
	                           &dfCellMax, &r2, &g2, &b2,
			           &sGrassColors, i);

	    GDALRATSetValueAsDouble(hrat, i, 0, dfCellMin);
	    GDALRATSetValueAsDouble(hrat, i, 1, dfCellMax);
	    GDALRATSetValueAsInt(hrat, i, 2, r1);
	    GDALRATSetValueAsInt(hrat, i, 3, r2);
	    GDALRATSetValueAsInt(hrat, i, 4, g1);
	    GDALRATSetValueAsInt(hrat, i, 5, g2);
	    GDALRATSetValueAsInt(hrat, i, 6, b1);
	    GDALRATSetValueAsInt(hrat, i, 7, b2);
	}

	if (GDALSetDefaultRAT(hBand, hrat) != CE_None) {
	    G_warning(_("Failed to set raster attribute table"));
	    ret = -1;
	}
	/* GDALRATDumpReadable(hrat, stdout); */

	GDALDestroyRasterAttributeTable(hrat);
    }
    
    Rast_free_cats(&cats);
    Rast_free_colors(&sGrassColors);

    return ret;
}
