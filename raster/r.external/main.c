
/****************************************************************************
 *
 * MODULE:       r.external
 *               
 * AUTHOR(S):    Glynn Clements, based on r.in.gdal
 *
 * PURPOSE:      Link raster map into GRASS utilizing the GDAL library.
 *
 * COPYRIGHT:    (C) 2008, 2010 by Glynn Clements and the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/imagery.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>

#include "gdal.h"
#include "ogr_srs_api.h"

#undef MIN
#undef MAX
#define MIN(a,b)      ((a) < (b) ? (a) : (b))
#define MAX(a,b)      ((a) > (b) ? (a) : (b))

struct band_info
{
    RASTER_MAP_TYPE data_type;
    GDALDataType gdal_type;
    int has_null;
    double null_val;
    double range[2];
    struct Colors colors;
};

enum flip {
    FLIP_H = 1,
    FLIP_V = 2,
};

static void list_formats(void)
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

static void check_projection(struct Cell_head *cellhd, GDALDatasetH hDS, int override)
{
    struct Cell_head loc_wind;
    struct Key_Value *proj_info = NULL, *proj_units = NULL;
    struct Key_Value *loc_proj_info = NULL, *loc_proj_units = NULL;
    int projcomp_error = 0;
    char error_msg[8096];

    /* Projection only required for checking so convert non-interactively */
    if (GPJ_wkt_to_grass(cellhd, &proj_info,
			 &proj_units, GDALGetProjectionRef(hDS), 0) < 0)
	G_warning(_("Unable to convert input raster map projection information to "
		    "GRASS format for checking"));
    else {
	/* -------------------------------------------------------------------- */
	/*      Does the projection of the current location match the           */
	/*      dataset?                                                        */
	/* -------------------------------------------------------------------- */
	G_get_window(&loc_wind);
	if (loc_wind.proj != PROJECTION_XY) {
	    loc_proj_info = G_get_projinfo();
	    loc_proj_units = G_get_projunits();
	}

	if (override) {
	    cellhd->proj = loc_wind.proj;
	    cellhd->zone = loc_wind.zone;
	    G_warning(_("Over-riding projection check"));
	}
	else if (loc_wind.proj != cellhd->proj ||
		 (projcomp_error = G_compare_projections(
		      loc_proj_info, loc_proj_units, proj_info, proj_units)) < 0) {
	    int i_value;

	    strcpy(error_msg,
		   _("Projection of dataset does not"
		     " appear to match current location.\n\n"));

	    /* TODO: output this info sorted by key: */
	    if (loc_proj_info != NULL) {
		strcat(error_msg, _("Location PROJ_INFO is:\n"));
		for (i_value = 0;
		     loc_proj_info != NULL &&
			 i_value < loc_proj_info->nitems; i_value++)
		    sprintf(error_msg + strlen(error_msg), "%s: %s\n",
			    loc_proj_info->key[i_value],
			    loc_proj_info->value[i_value]);
		strcat(error_msg, "\n");
	    }

	    if (proj_info != NULL) {
		strcat(error_msg, _("Dataset PROJ_INFO is:\n"));
		for (i_value = 0;
		     proj_info != NULL && i_value < proj_info->nitems;
		     i_value++)
		    sprintf(error_msg + strlen(error_msg), "%s: %s\n",
			    proj_info->key[i_value],
			    proj_info->value[i_value]);
		strcat(error_msg, "\nERROR: ");
		switch (projcomp_error) {
		case -1:
		    strcat(error_msg, "proj\n");
		    break;
		case -2:
		    strcat(error_msg, "units\n");
		    break;
		case -3:
		    strcat(error_msg, "datum\n");
		    break;
		case -4:
		    strcat(error_msg, "ellps\n");
		    break;
		case -5:
		    strcat(error_msg, "zone\n");
		    break;
		}
	    }
	    else {
		strcat(error_msg, _("Import dataset PROJ_INFO is:\n"));
		if (cellhd->proj == PROJECTION_XY)
		    sprintf(error_msg + strlen(error_msg),
			    "cellhd.proj = %d (unreferenced/unknown)\n",
			    cellhd->proj);
		else if (cellhd->proj == PROJECTION_LL)
		    sprintf(error_msg + strlen(error_msg),
			    "cellhd.proj = %d (lat/long)\n", cellhd->proj);
		else if (cellhd->proj == PROJECTION_UTM)
		    sprintf(error_msg + strlen(error_msg),
			    "cellhd.proj = %d (UTM), zone = %d\n",
			    cellhd->proj, cellhd->zone);
		else if (cellhd->proj == PROJECTION_SP)
		    sprintf(error_msg + strlen(error_msg),
			    "cellhd.proj = %d (State Plane), zone = %d\n",
			    cellhd->proj, cellhd->zone);
		else
		    sprintf(error_msg + strlen(error_msg),
			    "cellhd.proj = %d (unknown), zone = %d\n",
			    cellhd->proj, cellhd->zone);
	    }
	    strcat(error_msg,
		   _("\nYou can use the -o flag to r.external to override this check and "
		     "use the location definition for the dataset.\n"));
	    strcat(error_msg,
		   _("Consider generating a new location from the input dataset using "
		     "the 'location' parameter.\n"));
	    G_fatal_error(error_msg);
	}
	else {
	    G_message(_("Projection of input dataset and current location "
			"appear to match"));
	}
    }
}

static void setup_window(struct Cell_head *cellhd, GDALDatasetH hDS, int *flip)
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

static void update_default_window(struct Cell_head *cellhd)
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

static void query_band(GDALRasterBandH hBand, const char *output, int exact_range,
		       struct Cell_head *cellhd, struct band_info *info)
{
    int bGotMin, bGotMax;

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

    info->range[0] = GDALGetRasterMinimum(hBand, &bGotMin);
    info->range[1] = GDALGetRasterMaximum(hBand, &bGotMax);
    if(!(bGotMin && bGotMax))
	GDALComputeRasterMinMax(hBand, !exact_range, info->range);

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
	else  {
	    /* set data range to grey scale: */
	    G_verbose_message(_("Setting grey color table for <%s> (data range)"),
			      output);
	    Rast_make_grey_scale_colors(&info->colors,
				     (int) info->range[0], (int) info->range[1]);
	}
    }
}

static void make_cell(const char *output, const struct band_info *info)
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

static void make_link(const char *input, const char *output, int band,
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

static void write_fp_format(const char *output, const struct band_info *info)
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

static void write_fp_quant(const char *output)
{
    struct Quant quant;

    Rast_quant_init(&quant);
    Rast_quant_round(&quant);

    Rast_write_quant(output, G_mapset(), &quant);
}

static void create_map(const char *input, int band, const char *output,
		       struct Cell_head *cellhd, struct band_info *info,
		       const char *title, int flip)
{
    struct History history;
    struct Categories cats;

    Rast_put_cellhd(output, cellhd);

    make_cell(output, info);

    make_link(input, output, band, info, flip);

    if (info->data_type == CELL_TYPE) {
	struct Range range;
	range.min = (CELL)info->range[0];
	range.max = (CELL)info->range[1];
	range.first_time = 0;
	Rast_write_range(output, &range);
    }
    else {
	struct FPRange fprange;
	fprange.min = info->range[0];
	fprange.max = info->range[1];
	fprange.first_time = 0;
	Rast_write_fp_range(output, &fprange);
	write_fp_format(output, info);
	write_fp_quant(output);
    }

    G_verbose_message(_("Creating support files for %s"), output);
    Rast_short_history(output, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(output, &history);

    Rast_write_colors(output, G_mapset(), &info->colors);
    Rast_init_cats(NULL, &cats);
    Rast_write_cats((char *)output, &cats);

    if (title)
	Rast_put_cell_title(output, title);

    G_message(_("Link to raster map <%s> created"), output);
}

int main(int argc, char *argv[])
{
    char *input;
    char *source;
    char *output;
    char *title;
    struct Cell_head cellhd;
    GDALDatasetH hDS;
    GDALRasterBandH hBand;
    struct GModule *module;
    struct {
	struct Option *input, *source, *output, *band, *title;
    } parm;
    struct Flag *flag_o, *flag_f, *flag_e, *flag_r, *flag_h, *flag_v;
    int min_band, max_band, band;
    struct band_info info;
    int flip;
    struct Ref reference;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("import"));
    G_add_keyword(_("input"));
    G_add_keyword(_("external"));
    module->description =
	_("Link GDAL supported raster data as a pseudo GRASS raster map.");

    parm.input = G_define_standard_option(G_OPT_F_INPUT);
    parm.input->description = _("Raster file to be linked");
    
    parm.source = G_define_option();
    parm.source->key = "source";
    parm.source->description = _("Name of non-file GDAL data source");
    parm.source->required = NO;
    parm.source->type = TYPE_STRING;

    parm.output = G_define_standard_option(G_OPT_R_OUTPUT);
    
    parm.band = G_define_option();
    parm.band->key = "band";
    parm.band->type = TYPE_INTEGER;
    parm.band->required = NO;
    parm.band->description = _("Band to select (default: all)");

    parm.title = G_define_option();
    parm.title->key = "title";
    parm.title->key_desc = "phrase";
    parm.title->type = TYPE_STRING;
    parm.title->required = NO;
    parm.title->description = _("Title for resultant raster map");
    parm.title->guisection = _("Metadata");

    flag_o = G_define_flag();
    flag_o->key = 'o';
    flag_o->description =
	_("Override projection (use location's projection)");

    flag_e = G_define_flag();
    flag_e->key = 'e';
    flag_e->description = _("Extend location extents based on new dataset");

    flag_r = G_define_flag();
    flag_r->key = 'r';
    flag_r->description = _("Require exact range");

    flag_f = G_define_flag();
    flag_f->key = 'f';
    flag_f->description = _("List supported formats and exit");
    flag_f->guisection = _("Print");
    flag_f->suppress_required = YES;
    
    flag_h = G_define_flag();
    flag_h->key = 'h';
    flag_h->description = _("Flip horizontally");

    flag_v = G_define_flag();
    flag_v->key = 'v';
    flag_v->description = _("Flip vertically");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    GDALAllRegister();

    if (flag_f->answer) {
	list_formats();
	exit(EXIT_SUCCESS);
    }

    input = parm.input->answer;
    source = parm.source->answer;
    output = parm.output->answer;

    flip = 0;
    if (flag_h->answer)
	flip |= FLIP_H;
    if (flag_v->answer)
	flip |= FLIP_V;

    if (parm.title->answer) {
	title = G_store(parm.title->answer);
	G_strip(title);
    }
    else
	title = NULL;

    if (!input && !source)
	G_fatal_error(_("Name for input source not specified"));

    if (input && source)
	G_fatal_error(_("input= and source= are mutually exclusive"));

    if (!output)
	G_fatal_error(_("Name for output raster map not specified"));

    if (input && !G_is_absolute_path(input)) {
	char path[GPATH_MAX];
	getcwd(path, sizeof(path));
	strcat(path, "/");
	strcat(path, input);
	input = G_store(path);
    }

    if (!input)
	input = source;

    hDS = GDALOpen(input, GA_ReadOnly);
    if (hDS == NULL)
	return 1;

    setup_window(&cellhd, hDS, &flip);

    check_projection(&cellhd, hDS, flag_o->answer);

    Rast_set_window(&cellhd);

    if (parm.band->answer)
	min_band = max_band = atoi(parm.band->answer);
    else
	min_band = 1, max_band = GDALGetRasterCount(hDS);

    G_verbose_message(_("Proceeding with import..."));

    if (max_band > min_band) {
	if (I_find_group(output) == 1)
	    G_warning(_("Imagery group <%s> already exists and will be overwritten."), output);
	I_init_group_ref(&reference);
    }

    for (band = min_band; band <= max_band; band++) {
	char *output2, *title2 = NULL;

	G_message("Importing band %d of %d...", band, GDALGetRasterCount( hDS ));

	hBand = GDALGetRasterBand(hDS, band);
	if (!hBand)
	    G_fatal_error(_("Selected band (%d) does not exist"), band);

	if (max_band > min_band) {
	    G_asprintf(&output2, "%s.%d", output, band);
	    if (title)
		G_asprintf(&title2, "%s (band %d)", title, band);
	    G_debug(1, "Adding raster map <%s> to group <%s>", output2, output);
	    I_add_file_to_group_ref(output2, G_mapset(), &reference);
	}
	else {
	    output2 = G_store(output);
	    if (title)
		title2 = G_store(title);
	}

	query_band(hBand, output2, flag_r->answer, &cellhd, &info);
	create_map(input, band, output2, &cellhd, &info, title, flip);

	G_free(output2);
	G_free(title2);
    }

    if (flag_e->answer)
	update_default_window(&cellhd);

    /* Create the imagery group if multiple bands are imported */
    if (max_band > min_band) {
    	I_put_group_ref(output, &reference);
	I_put_group(output);
	G_message(_("Imagery group <%s> created"), output);
    }

    exit(EXIT_SUCCESS);
}

