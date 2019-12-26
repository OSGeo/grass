#include <grass/gis.h>
#include <grass/glocale.h>

#include <gdal.h>

#include "proto.h"

void query_band(GDALRasterBandH hBand, const char *output,
		struct Cell_head *cellhd, struct band_info *info)
{
    info->gdal_type = GDALGetRasterDataType(hBand);

    info->null_val = GDALGetRasterNoDataValue(hBand, &info->has_null);

    cellhd->compressed = 0;

    switch (info->gdal_type) {
    case GDT_Float32:
	info->data_type = FCELL_TYPE;
	cellhd->format = -1;
	break;

    case GDT_Float64:
	info->data_type = DCELL_TYPE;
	cellhd->format = -1;
	break;

    case GDT_Byte:
	info->data_type = CELL_TYPE;
	cellhd->format = 0;
	break;

    case GDT_Int16:
    case GDT_UInt16:
	info->data_type = CELL_TYPE;
	cellhd->format = 1;
	break;

    case GDT_Int32:
    case GDT_UInt32:
	info->data_type = CELL_TYPE;
	cellhd->format = 3;
	break;

    default:
	G_fatal_error(_("Complex types not supported"));
	break;
    }

    Rast_init_colors(&info->colors);

    if (GDALGetRasterColorTable(hBand) != NULL) {
	GDALColorTableH hCT;
	int count, i;

	G_verbose_message(_("Copying color table for %s"), output);

	hCT = GDALGetRasterColorTable(hBand);
	count = GDALGetColorEntryCount(hCT);

	for (i = 0; i < count; i++) {
	    GDALColorEntry sEntry;

	    GDALGetColorEntryAsRGB(hCT, i, &sEntry);
	    if (sEntry.c4 == 0)
		continue;

	    Rast_set_c_color(i, sEntry.c1, sEntry.c2, sEntry.c3, &info->colors);
	}
    }
    else {
	if (info->gdal_type == GDT_Byte) {
	    /* set full 0..255 range to grey scale: */
	    G_verbose_message(_("Setting grey color table for <%s> (full 8bit range)"),
			      output);
	    Rast_make_grey_scale_colors(&info->colors, 0, 255);
	}
    }
}

void make_cell(const char *output, const struct band_info *info)
{
    FILE *fp;

    fp = G_fopen_new("cell", output);
    if (!fp)
	G_fatal_error(_("Unable to create cell/%s file"), output);

    fclose(fp);

    if (info->data_type == CELL_TYPE)
	return;

    fp = G_fopen_new("fcell", output);
    if (!fp)
	G_fatal_error(_("Unable to create fcell/%s file"), output);

    fclose(fp);
}

void make_link(const char *input, const char *output, int band,
	       const struct band_info *info, int flip)
{
    struct Key_Value *key_val = G_create_key_value();
    char null_str[256], type_str[8], band_str[8];
    FILE *fp;

    sprintf(band_str, "%d", band);

    if (info->has_null) {
	if (info->data_type == CELL_TYPE)
	    sprintf(null_str, "%d", (int) info->null_val);
	else
	    sprintf(null_str, "%.22g", info->null_val);
    }
    else
	strcpy(null_str, "none");

    sprintf(type_str, "%d", info->gdal_type);

    G_set_key_value("file", input, key_val);
    G_set_key_value("band", band_str, key_val);
    G_set_key_value("null", null_str, key_val);
    G_set_key_value("type", type_str, key_val);
    if (flip & FLIP_H)
	G_set_key_value("hflip", "yes", key_val);
    if (flip & FLIP_V)
	G_set_key_value("vflip", "yes", key_val);

    fp = G_fopen_new_misc("cell_misc", "gdal", output);
    if (!fp)
	G_fatal_error(_("Unable to create cell_misc/%s/gdal file"), output);

    if (G_fwrite_key_value(fp, key_val) < 0)
	G_fatal_error(_("Error writing cell_misc/%s/gdal file"), output);

    fclose(fp);
}

void write_fp_format(const char *output, const struct band_info *info)
{
    struct Key_Value *key_val;
    const char *type;
    FILE *fp;

    if (info->data_type == CELL_TYPE)
	return;

    key_val = G_create_key_value();

    type = (info->data_type == FCELL_TYPE)
	? "float"
	: "double";
    G_set_key_value("type", type, key_val);

    G_set_key_value("byte_order", "xdr", key_val);

    fp = G_fopen_new_misc("cell_misc", "f_format", output);
    if (!fp)
	G_fatal_error(_("Unable to create cell_misc/%s/f_format file"), output);

    if (G_fwrite_key_value(fp, key_val) < 0)
	G_fatal_error(_("Error writing cell_misc/%s/f_format file"), output);

    fclose(fp);

    G_free_key_value(key_val);
}

void write_fp_quant(const char *output)
{
    struct Quant quant;

    Rast_quant_init(&quant);
    Rast_quant_round(&quant);

    Rast_write_quant(output, G_mapset(), &quant);
}

void create_map(const char *input, int band, const char *output,
		struct Cell_head *cellhd, struct band_info *info,
		const char *title, int flip)
{
    struct History history;
    struct Categories cats;
    char buf[1024];
    int outfd;

    Rast_put_cellhd(output, cellhd);

    make_cell(output, info);

    make_link(input, output, band, info, flip);

    if (info->data_type != CELL_TYPE) {
	write_fp_format(output, info);
	write_fp_quant(output);
    }

    G_verbose_message(_("Creating support files for %s"), output);
    Rast_short_history(output, "GDAL-link", &history);
    Rast_command_history(&history);
    sprintf(buf, "%s band %d", input, band);
    Rast_set_history(&history, HIST_DATSRC_1, buf);
    Rast_write_history(output, &history);

    Rast_write_colors(output, G_mapset(), &info->colors);
    Rast_init_cats(NULL, &cats);
    Rast_write_cats((char *)output, &cats);

    if (title)
	Rast_put_cell_title(output, title);

    /* get stats for this raster band */
    G_remove_misc("cell_misc", "stats", output);

    outfd = Rast_open_old(output, G_mapset());
    if (info->data_type == CELL_TYPE) {
	int r;
	struct Range range;
	CELL *rbuf = Rast_allocate_buf(CELL_TYPE);

	G_remove_misc("cell_misc", "range", output);
	Rast_init_range(&range);
	for (r = 0; r < cellhd->rows; r++) {
	    Rast_get_row(outfd, rbuf, r, CELL_TYPE);
	    Rast_row_update_range(rbuf, cellhd->cols, &range);
	}
	Rast_write_range(output, &range);
	G_free(rbuf);
    }
    else {
	int r;
	struct FPRange fprange;
	void *rbuf = Rast_allocate_buf(info->data_type);
	
	G_remove_misc("cell_misc", "f_range", output);
	Rast_init_fp_range(&fprange);
	for (r = 0; r < cellhd->rows; r++) {
	    Rast_get_row(outfd, rbuf, r, info->data_type);
	    Rast_row_update_fp_range(rbuf, cellhd->cols, &fprange, info->data_type);
	}
	Rast_write_fp_range(output, &fprange);
	G_free(rbuf);
    }
    Rast_unopen(outfd);

    G_message(_("Link to raster map <%s> created."), output);
}
