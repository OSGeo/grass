#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include <gdal.h>

/* -------------------------------------------------------------------- */
/*      Transfer colormap, if there is one.                             */
/*      prefer color rules over color tables, search:                   */
/*      1. GRASS color rules in metadata                                */
/*      2. Raster attribute table with color rules                      */
/*      3. Raster color table                                           */
/* -------------------------------------------------------------------- */
void transfer_colormap(GDALRasterBandH hBand, const char *output)
{
    int have_colors = 0;
    int nrows, ncols;
    int indx;
    int complex = FALSE; /* TODO */

    char **GDALmetadata;
    GDALRasterAttributeTableH gdal_rat;
    
    /* GRASS color rules in metadata? */
    GDALmetadata = GDALGetMetadata(hBand, "");
    
    if (GDALmetadata) {
        struct Colors colors;
        DCELL val1, val2;
        int r1, g1, b1, r2, g2, b2;
        
        Rast_init_colors(&colors);
        
        while (GDALmetadata && GDALmetadata[0]) {
            G_debug(2, "%s", GDALmetadata[0]);
            
	    if (!strncmp("COLOR_TABLE_RULE_RGB_", GDALmetadata[0], 21)) {
		char *p;
		
		for (p = GDALmetadata[0]; *p != '=' && *p != '\0'; p++);
		
		if (*p == '=') {
		    p++;
		}
		if (p && *p != '\0') {
		    if (sscanf(p, "%lf %lf %d %d %d %d %d %d",
			&val1, &val2, &r1, &g1, &b1, &r2, &g2, &b2) == 8) {

			Rast_add_d_color_rule(&val1, r1, g1, b1,
			                      &val2, r2, g2, b2,
					      &colors);
			have_colors = 1;
		    }
		}
	    }
	    GDALmetadata++;
	}
	if (have_colors)
	    Rast_write_colors((char *)output, G_mapset(), &colors);

	Rast_free_colors(&colors);
    }

    /* colors in raster attribute table? */
    gdal_rat = GDALGetDefaultRAT(hBand);
    if (!have_colors && gdal_rat != NULL) {
	nrows = GDALRATGetRowCount(gdal_rat);
	ncols = GDALRATGetColumnCount(gdal_rat);
	
	if (nrows > 0 && ncols > 0) {
	    int minc, maxc, minmaxc;
	    int rc, gc, bc, rminc, rmaxc, gminc, gmaxc, bminc, bmaxc;
	    GDALRATFieldUsage field_use;
	    struct Colors colors;
	    DCELL val1, val2;
	    double r1, g1, b1, r2, g2, b2;
	    int cf;

	    Rast_init_colors(&colors);
	    
	    minc = maxc = minmaxc = -1;
	    rc = gc = bc = rminc = rmaxc = gminc = gmaxc = bminc = bmaxc = -1;

	    for (indx = 0; indx < ncols; indx++) {
		 field_use = GDALRATGetUsageOfCol(gdal_rat, indx);
		 
		 if (field_use == GFU_Min)
		    minc = indx;
		 else if (field_use == GFU_Max)
		    maxc = indx;
		 else if (field_use == GFU_MinMax)
		    minmaxc = indx;
		 else if (field_use == GFU_Red)
		    rc = indx;
		 else if (field_use == GFU_Green)
		    gc = indx;
		 else if (field_use == GFU_Blue)
		    bc = indx;
		 else if (field_use == GFU_RedMin)
		    rminc = indx;
		 else if (field_use == GFU_GreenMin)
		    gminc = indx;
		 else if (field_use == GFU_BlueMin)
		    bminc = indx;
		 else if (field_use == GFU_RedMax)
		    rmaxc = indx;
		 else if (field_use == GFU_GreenMax)
		    gmaxc = indx;
		 else if (field_use == GFU_BlueMax)
		    bmaxc = indx;
	    }
	    
	    /* guess color range 0, 1 or 0, 255 */

	    if (minc >= 0 && maxc >= 0 && rminc >= 0 && rmaxc >= 0 &&
		gminc >= 0 && gmaxc >= 0 && bminc >= 0 && bmaxc >= 0) {

		cf = 1;
		
		/* analyze color rules */
		for (indx = 0; indx < nrows; indx++) {
		    val1 = GDALRATGetValueAsDouble(gdal_rat, indx, minc);
		    val2 = GDALRATGetValueAsDouble(gdal_rat, indx, maxc);

		    r1 = GDALRATGetValueAsDouble(gdal_rat, indx, rminc);
		    g1 = GDALRATGetValueAsDouble(gdal_rat, indx, gminc);
		    b1 = GDALRATGetValueAsDouble(gdal_rat, indx, bminc);

		    r2 = GDALRATGetValueAsDouble(gdal_rat, indx, rmaxc);
		    g2 = GDALRATGetValueAsDouble(gdal_rat, indx, gmaxc);
		    b2 = GDALRATGetValueAsDouble(gdal_rat, indx, bmaxc);

		    if (r1 > 0.0 && r1 < 1.0)
			cf = 255;
		    else if (cf == 255 && r1 > 1.0) {
			cf = 0;
			break;
		    }

		    if (g1 > 0.0 && g1 < 1.0)
			cf = 255;
		    else if (cf == 255 && g1 > 1.0) {
			cf = 0;
			break;
		    }

		    if (b1 > 0.0 && b1 < 1.0)
			cf = 255;
		    else if (cf == 255 && b1 > 1.0) {
			cf = 0;
			break;
		    }

		    if (r2 > 0.0 && r2 < 1.0)
			cf = 255;
		    else if (cf == 255 && r2 > 1.0) {
			cf = 0;
			break;
		    }

		    if (g2 > 0.0 && g2 < 1.0)
			cf = 255;
		    else if (cf == 255 && g2 > 1.0) {
			cf = 0;
			break;
		    }

		    if (b2 > 0.0 && b2 < 1.0)
			cf = 255;
		    else if (cf == 255 && b2 > 1.0) {
			cf = 0;
			break;
		    }
		}

		if (cf == 0)
		    G_warning(_("Inconsistent color rules in RAT"));
		else {
		    /* fetch color rules */
		    for (indx = 0; indx < nrows; indx++) {
			val1 = GDALRATGetValueAsDouble(gdal_rat, indx, minc);
			val2 = GDALRATGetValueAsDouble(gdal_rat, indx, maxc);

			r1 = GDALRATGetValueAsDouble(gdal_rat, indx, rminc);
			g1 = GDALRATGetValueAsDouble(gdal_rat, indx, gminc);
			b1 = GDALRATGetValueAsDouble(gdal_rat, indx, bminc);

			r2 = GDALRATGetValueAsDouble(gdal_rat, indx, rmaxc);
			g2 = GDALRATGetValueAsDouble(gdal_rat, indx, gmaxc);
			b2 = GDALRATGetValueAsDouble(gdal_rat, indx, bmaxc);

			Rast_add_d_color_rule(&val1, r1 * cf, g1 * cf, b1 * cf,
					      &val2, r2 * cf, g2 * cf, b2 * cf,
					      &colors);
		    }
		}
	    }
	    else if (minmaxc >= 0 && rc >= 0 && gc >= 0 && bc >= 0) {
		
		cf = 1;

		/* analyze color table */
		for (indx = 0; indx < nrows; indx++) {
		    val1 = GDALRATGetValueAsDouble(gdal_rat, indx, minmaxc);

		    r1 = GDALRATGetValueAsDouble(gdal_rat, indx, rc);
		    g1 = GDALRATGetValueAsDouble(gdal_rat, indx, gc);
		    b1 = GDALRATGetValueAsDouble(gdal_rat, indx, bc);


		    if (r1 > 0.0 && r1 < 1.0)
			cf = 255;
		    else if (cf == 255 && r1 > 1.0) {
			cf = 0;
			break;
		    }

		    if (g1 > 0.0 && g1 < 1.0)
			cf = 255;
		    else if (cf == 255 && g1 > 1.0) {
			cf = 0;
			break;
		    }

		    if (b1 > 0.0 && b1 < 1.0)
			cf = 255;
		    else if (cf == 255 && b1 > 1.0) {
			cf = 0;
			break;
		    }
		}

		if (cf == 0)
		    G_warning(_("Inconsistent color rules in RAT"));
		else {
		    /* fetch color table */
		    for (indx = 0; indx < nrows; indx++) {
			val1 = GDALRATGetValueAsDouble(gdal_rat, indx, minmaxc);

			r1 = GDALRATGetValueAsDouble(gdal_rat, indx, rc);
			g1 = GDALRATGetValueAsDouble(gdal_rat, indx, gc);
			b1 = GDALRATGetValueAsDouble(gdal_rat, indx, bc);
			
			Rast_set_d_color(val1, r1 * cf, g1 * cf, b1 * cf, &colors);
		    }
		}
	    }
	    
	    have_colors = Rast_colors_count(&colors) > 0;
	    
	    if (have_colors)
		Rast_write_colors((char *)output, G_mapset(), &colors);

	    Rast_free_colors(&colors);
	}
    }

    /* colors in raster color table? */

    if (!have_colors && !complex && GDALGetRasterColorTable(hBand) != NULL) {
	GDALColorTableH hCT;
	struct Colors colors;
	int iColor;

	G_debug(1, "Copying color table for %s", output);

	hCT = GDALGetRasterColorTable(hBand);

	Rast_init_colors(&colors);
	for (iColor = 0; iColor < GDALGetColorEntryCount(hCT); iColor++) {
	    GDALColorEntry sEntry;

	    GDALGetColorEntryAsRGB(hCT, iColor, &sEntry);
	    if (sEntry.c4 == 0)
		continue;

	    Rast_set_c_color(iColor, sEntry.c1, sEntry.c2, sEntry.c3, &colors);
	}

	Rast_write_colors((char *)output, G_mapset(), &colors);
	Rast_free_colors(&colors);
	have_colors = 1;
    }
    if (!have_colors) {			/* no color table present */

	/* types are defined in GDAL: ./core/gdal.h */
	if ((GDALGetRasterDataType(hBand) == GDT_Byte)) {
	    /* found 0..255 data: we set to grey scale: */
	    struct Colors colors;

	    G_verbose_message(_("Setting grey color table for <%s> (8bit, full range)"),
			      output);

	    Rast_init_colors(&colors);
	    Rast_make_grey_scale_colors(&colors, 0, 255);	/* full range */
	    Rast_write_colors((char *)output, G_mapset(), &colors);
	    Rast_free_colors(&colors);
	}
	if ((GDALGetRasterDataType(hBand) == GDT_UInt16)) {
	    /* found 0..65535 data: we set to grey scale: */
	    struct Colors colors;
	    struct Range range;
	    CELL min, max;

	    G_verbose_message(_("Setting grey color table for <%s> (16bit, image range)"),
			      output);
	    Rast_read_range((char *)output, G_mapset(), &range);
	    Rast_get_range_min_max(&range, &min, &max);

	    Rast_init_colors(&colors);
	    Rast_make_grey_scale_colors(&colors, min, max);	/* image range */
	    Rast_write_colors((char *)output, G_mapset(), &colors);
	    Rast_free_colors(&colors);
	}
    }
}
