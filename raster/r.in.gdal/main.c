
/****************************************************************************
 *
 * MODULE:       r.in.gdal
 *               
 * AUTHOR(S):    Frank Warmerdam (copyright of this file)
 *               Added optional GCP transformation: Markus Neteler 10/2001
 *
 * PURPOSE:      Imports many GIS/image formats into GRASS utilizing the GDAL
 *               library.
 *
 * COPYRIGHT:    (C) 2001-2015 by Frank Warmerdam, and the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
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

static void ImportBand(GDALRasterBandH hBand, const char *output,
		       struct Ref *group_ref);
static void SetupReprojector(const char *pszSrcWKT, const char *pszDstLoc,
			     struct pj_info *iproj, struct pj_info *oproj);
static int dump_rat(GDALRasterBandH hBand, char *outrat, int nBand);
static void error_handler_ds(void *p);
static int l1bdriver;

/************************************************************************/
/*                                main()                                */

/************************************************************************/

int main(int argc, char *argv[])
{
    char *input;
    char *output;
    char *title;
    struct Cell_head cellhd, loc_wind, cur_wind;
    struct Key_Value *proj_info = NULL, *proj_units = NULL;
    struct Key_Value *loc_proj_info = NULL, *loc_proj_units = NULL;
    GDALDatasetH hDS;
    GDALDriverH hDriver;
    GDALRasterBandH hBand;
    double adfGeoTransform[6];
    int n_bands;
    int force_imagery = FALSE;
    char error_msg[8096];
    int projcomp_error = 0;
    int overwrite;
    int offset = 0;
    char *suffix;
    int num_digits = 0;

    struct GModule *module;
    struct
    {
	struct Option *input, *output, *target, *title, *outloc, *band,
	              *memory, *offset, *num_digits, *map_names_file,
	              *rat;
    } parm;
    struct Flag *flag_o, *flag_e, *flag_k, *flag_f, *flag_l, *flag_c, *flag_p,
        *flag_j;

    /* -------------------------------------------------------------------- */
    /*      Initialize.                                                     */
    /* -------------------------------------------------------------------- */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("import"));
    module->description =
	_("Imports raster data into a GRASS raster map using GDAL library.");

    /* -------------------------------------------------------------------- */
    /*      Setup and fetch parameters.                                     */
    /* -------------------------------------------------------------------- */
    parm.input = G_define_standard_option(G_OPT_F_INPUT);
    parm.input->description = _("Name of raster file to be imported");
    
    parm.output = G_define_standard_option(G_OPT_R_OUTPUT);

    parm.band = G_define_option();
    parm.band->key = "band";
    parm.band->type = TYPE_INTEGER;
    parm.band->multiple = YES;
    parm.band->required = NO;
    parm.band->description = _("Band(s) to select (default is all bands)");

    parm.memory = G_define_option();
    parm.memory->key = "memory";
    parm.memory->type = TYPE_INTEGER;
    parm.memory->required = NO;
    parm.memory->options = "0-2047";
    parm.memory->answer = "300";
    parm.memory->label = _("Maximum memory to be used (in MB)");
    parm.memory->description = _("Cache size for raster rows");

    parm.target = G_define_option();
    parm.target->key = "target";
    parm.target->type = TYPE_STRING;
    parm.target->required = NO;
    parm.target->label = _("Name of GCPs target location");
    parm.target->description =
	_("Name of location to create or to read projection from for GCPs transformation");
    parm.target->key_desc = "name";

    parm.title = G_define_option();
    parm.title->key = "title";
    parm.title->key_desc = "phrase";
    parm.title->type = TYPE_STRING;
    parm.title->required = NO;
    parm.title->description = _("Title for resultant raster map");
    parm.title->guisection = _("Metadata");

    parm.offset = G_define_option();
    parm.offset->key = "offset";
    parm.offset->type = TYPE_INTEGER;
    parm.offset->required = NO;
    parm.offset->answer = "0";
    parm.offset->label = _("Offset to be added to band numbers");
    parm.offset->description = _("If 0, no offset is added and the first band is 1");
    parm.offset->guisection = _("Metadata");

    parm.num_digits = G_define_option();
    parm.num_digits->key = "num_digits";
    parm.num_digits->type = TYPE_INTEGER;
    parm.num_digits->required = NO;
    parm.num_digits->answer = "0";
    parm.num_digits->label = _("Zero-padding of band number by filling with leading zeros up to given number");
    parm.num_digits->description = _("If 0, length will be adjusted to 'offset' number without leading zeros");
    parm.num_digits->guisection = _("Metadata");

    parm.map_names_file = G_define_standard_option(G_OPT_F_OUTPUT);
    parm.map_names_file->key = "map_names_file";
    parm.map_names_file->required = NO;
    parm.map_names_file->description = _("Name of the output file that contains the imported map names");
    parm.map_names_file->guisection = _("Metadata");

    parm.outloc = G_define_option();
    parm.outloc->key = "location";
    parm.outloc->type = TYPE_STRING;
    parm.outloc->required = NO;
    parm.outloc->description = _("Name for new location to create");
    parm.outloc->key_desc = "name";

    parm.rat = G_define_option();
    parm.rat->key = "table";
    parm.rat->type = TYPE_STRING;
    parm.rat->required = NO;
    parm.rat->label = _("File prefix for raster attribute tables");
    parm.rat->description = _("The band number and \".csv\" will be appended to the file prefix");
    parm.rat->key_desc = "file";

    flag_o = G_define_flag();
    flag_o->key = 'o';
    flag_o->label =
	_("Override projection check (use current location's projection)");
    flag_o->description =
	_("Assume that the dataset has same projection as the current location");

    flag_j = G_define_flag();
    flag_j->key = 'j';
    flag_j->description =
	_("Perform projection check only and exit");
    flag_j->suppress_required = YES;
    G_option_requires(flag_j, parm.input, NULL);
    
    flag_e = G_define_flag();
    flag_e->key = 'e';
    flag_e->label = _("Extend region extents based on new dataset");
    flag_e->description =
	_("Also updates the default region if in the PERMANENT mapset");

    flag_f = G_define_flag();
    flag_f->key = 'f';
    flag_f->description = _("List supported formats and exit");
    flag_f->guisection = _("Print");
    flag_f->suppress_required = YES;
    
    flag_l = G_define_flag();
    flag_l->key = 'l';
    flag_l->description =
	_("Force Lat/Lon maps to fit into geographic coordinates (90N,S; 180E,W)");

    flag_k = G_define_flag();
    flag_k->key = 'k';
    flag_k->description =
	_("Keep band numbers instead of using band color names");

    flag_c = G_define_flag();
    flag_c->key = 'c';
    flag_c->description =
	_("Create the location specified by the \"location\" parameter and exit."
          " Do not import the raster file.");

    flag_p = G_define_flag();
    flag_p->key = 'p';
    flag_p->description = _("Print number of bands and exit");
    flag_p->suppress_required = YES;
    G_option_requires(flag_p, parm.input, NULL);
    
    /* The parser checks if the map already exists in current mapset, this is
     * wrong if location options is used, so we switch out the check and do it
     * in the module after the parser */
    overwrite = G_check_overwrite(argc, argv);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    input = parm.input->answer;

    output = parm.output->answer;

    offset = atoi(parm.offset->answer);

    num_digits = atoi(parm.num_digits->answer);
    
    if ((title = parm.title->answer))
	G_strip(title);

    /* -------------------------------------------------------------------- */
    /*      Do some additional parameter validation.                        */
    /* -------------------------------------------------------------------- */
    if (parm.target->answer && parm.outloc->answer
	&& strcmp(parm.target->answer, parm.outloc->answer) == 0) {
	G_fatal_error(_("You have to specify a target location different from output location"));
    }

    if (flag_c->answer && parm.outloc->answer == NULL ) {
	G_fatal_error(_("You need to specify valid location name."));
    }

    if (flag_l->answer && G_projection() != PROJECTION_LL)
	G_fatal_error(_("The '-l' flag only works in Lat/Lon locations"));

    if(num_digits < 0)
        G_fatal_error(_("The number of digits for band numbering must be equal or greater than 0"));

    /* Allocate the suffix string */
    if(num_digits > 0) {
        suffix = G_calloc( num_digits + 1, sizeof(char));
    } else {
        /* Band number length should not exceed 64 digits */
        suffix = G_calloc(65, sizeof(char));
    }


    /* -------------------------------------------------------------------- */
    /*      Fire up the engines.                                            */
    /* -------------------------------------------------------------------- */
    GDALAllRegister();
    /* default GDAL memory cache size appears to be only 40 MiB, slowing down r.in.gdal */
    if (parm.memory->answer && *parm.memory->answer) {
	   /* TODO: GDALGetCacheMax() overflows at 2GiB, implement use of GDALSetCacheMax64() */
           GDALSetCacheMax(atol(parm.memory->answer) * 1024 * 1024);
           G_verbose_message(_("Using memory cache size: %.1f MiB"), GDALGetCacheMax()/1024.0/1024.0);
    }

    /* -------------------------------------------------------------------- */
    /*      List supported formats and exit.                                */
    /*         code from GDAL 1.2.5  gcore/gdal_misc.cpp                    */
    /*         Copyright (c) 1999, Frank Warmerdam                          */
    /* -------------------------------------------------------------------- */
    if (flag_f->answer) {
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
	exit(EXIT_SUCCESS);
    }

    /* -------------------------------------------------------------------- */
    /*      Open the file.                                                  */
    /* -------------------------------------------------------------------- */
    hDS = GDALOpen(input, GA_ReadOnly);
    if (hDS == NULL)
        G_fatal_error(_("Unable to open datasource <%s>"), input);
    G_add_error_handler(error_handler_ds, hDS);
    
    hDriver = GDALGetDatasetDriver(hDS);	/* needed for AVHRR data */
    /* L1B - NOAA/AVHRR data must be treated differently */
    /* for hDriver names see gdal/frmts/gdalallregister.cpp */
    G_debug(3, "GDAL Driver: %s", GDALGetDriverShortName(hDriver));
    if (strcmp(GDALGetDriverShortName(hDriver), "L1B") != 0)
	l1bdriver = 0;
    else {
	l1bdriver = 1;		/* AVHRR found, needs north south flip */
	G_warning(_("Input seems to be NOAA/AVHRR data which needs to be "
	            "georeferenced with thin plate spline transformation "
		    "(%s or %s)."), "i.rectify -t", "gdalwarp -tps");
    }

    if (flag_p->answer) {
        /* print number of bands */
        fprintf(stdout, "%d\n", GDALGetRasterCount(hDS));
        GDALClose(hDS);
        exit(EXIT_SUCCESS);
    }

    if (output && !parm.outloc->answer &&
        GDALGetRasterCount(hDS) == 1) {	/* Check if the map exists */
	if (G_find_raster2(output, G_mapset())) {
	    if (overwrite)
		G_warning(_("Raster map <%s> already exists and will be overwritten"),
			  output);
	    else
		G_fatal_error(_("Raster map <%s> already exists"), output);
	}
    }

    /* zero cell header */
    G_zero(&cellhd, sizeof(struct Cell_head));
    
    /* -------------------------------------------------------------------- */
    /*      Set up the window representing the data we have.                */
    /* -------------------------------------------------------------------- */
    G_debug(3, "GDAL size: row = %d, col = %d", GDALGetRasterYSize(hDS),
	    GDALGetRasterXSize(hDS));
    cellhd.rows = GDALGetRasterYSize(hDS);
    cellhd.rows3 = GDALGetRasterYSize(hDS);
    cellhd.cols = GDALGetRasterXSize(hDS);
    cellhd.cols3 = GDALGetRasterXSize(hDS);

    if (GDALGetGeoTransform(hDS, adfGeoTransform) == CE_None) {
	if (adfGeoTransform[2] != 0.0 || adfGeoTransform[4] != 0.0 ||
	    adfGeoTransform[1] <= 0.0 || adfGeoTransform[5] >= 0.0)
	    G_fatal_error(_("Input raster map is flipped or rotated - cannot import. "
			    "You may use 'gdalwarp' to transform the map to North-up."));
	cellhd.north = adfGeoTransform[3];
	cellhd.ns_res = fabs(adfGeoTransform[5]);
	cellhd.ns_res3 = fabs(adfGeoTransform[5]);
	cellhd.south = cellhd.north - cellhd.ns_res * cellhd.rows;
	cellhd.west = adfGeoTransform[0];
	cellhd.ew_res = fabs(adfGeoTransform[1]);
	cellhd.ew_res3 = fabs(adfGeoTransform[1]);
	cellhd.east = cellhd.west + cellhd.cols * cellhd.ew_res;
	cellhd.top = 1.;
	cellhd.bottom = 0.;
	cellhd.tb_res = 1.;
	cellhd.depths = 1;
    }
    else {
	cellhd.north = cellhd.rows;
	cellhd.south = 0.0;
	cellhd.ns_res = 1.0;
	cellhd.ns_res3 = 1.0;
	cellhd.west = 0.0;
	cellhd.east = cellhd.cols;
	cellhd.ew_res = 1.0;
	cellhd.ew_res3 = 1.0;
	cellhd.top = 1.;
	cellhd.bottom = 0.;
	cellhd.tb_res = 1.;
	cellhd.depths = 1;
    }

    /* constrain to geographic coords */
    if (flag_l->answer && G_projection() == PROJECTION_LL) {
	if (cellhd.north > 90.) cellhd.north = 90.;
	if (cellhd.south < -90.) cellhd.south = -90.;
	if (cellhd.east > 360.) cellhd.east = 180.;
	if (cellhd.west < -180.) cellhd.west = -180.;
	cellhd.ns_res = (cellhd.north - cellhd.south) / cellhd.rows;
	cellhd.ew_res = (cellhd.east - cellhd.west) / cellhd.cols;
	cellhd.ew_res3 = cellhd.ew_res;
	cellhd.ns_res3 = cellhd.ns_res;

	G_warning(_("Map bounds have been constrained to geographic "
	    "coordinates. You will almost certainly want to check "
	    "map bounds and resolution with r.info and reset them "
	    "with r.region before going any further."));
    }

    /* -------------------------------------------------------------------- */
    /*      Fetch the projection in GRASS form.                             */
    /* -------------------------------------------------------------------- */
    proj_info = NULL;
    proj_units = NULL;

    /* -------------------------------------------------------------------- */
    /*      Do we need to create a new location?                            */
    /* -------------------------------------------------------------------- */
    if (parm.outloc->answer != NULL) {
	/* Convert projection information non-interactively as we can't
	 * assume the user has a terminal open */
	if (GPJ_wkt_to_grass(&cellhd, &proj_info,
			     &proj_units, GDALGetProjectionRef(hDS), 0) < 0) {
	    G_fatal_error(_("Unable to convert input map projection to GRASS "
			    "format; cannot create new location."));
	}
	else {
	    if (0 != G_make_location(parm.outloc->answer, &cellhd,
				     proj_info, proj_units)) {
		G_fatal_error(_("Unable to create new location <%s>"),
			      parm.outloc->answer);
	    }
	    G_message(_("Location <%s> created"), parm.outloc->answer);
	}

        /* If the c flag is set, clean up? and exit here */
        if (flag_c->answer) {
            exit(EXIT_SUCCESS);
        }
    }
    else {
	/* Projection only required for checking so convert non-interactively */
	if (GPJ_wkt_to_grass(&cellhd, &proj_info,
			     &proj_units, GDALGetProjectionRef(hDS), 0) < 0)
	    G_warning(_("Unable to convert input raster map projection information to "
		       "GRASS format for checking"));
	else {
            void (*msg_fn)(const char *, ...);

	    /* -------------------------------------------------------------------- */
	    /*      Does the projection of the current location match the           */
	    /*      dataset?                                                        */
	    /* -------------------------------------------------------------------- */
	    G_get_default_window(&loc_wind);
	    if (loc_wind.proj != PROJECTION_XY) {
		loc_proj_info = G_get_projinfo();
		loc_proj_units = G_get_projunits();
	    }

	    if (flag_o->answer) {
		cellhd.proj = loc_wind.proj;
		cellhd.zone = loc_wind.zone;
		G_warning(_("Over-riding projection check"));
	    }
	    else if (loc_wind.proj != cellhd.proj
		     || (projcomp_error = G_compare_projections(loc_proj_info,
								loc_proj_units,
								proj_info,
								proj_units)) < 0) {
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
		    case -6:
			strcat(error_msg, "south\n");
			break;
		    case -7:
			strcat(error_msg, "x_0\n");
			break;
		    case -8:
			strcat(error_msg, "y_0\n");
			break;
		    }
		}
		else {
		    strcat(error_msg, _("Import dataset PROJ_INFO is:\n"));
		    if (cellhd.proj == PROJECTION_XY)
			sprintf(error_msg + strlen(error_msg),
				"cellhd.proj = %d (unreferenced/unknown)\n",
				cellhd.proj);
		    else if (cellhd.proj == PROJECTION_LL)
			sprintf(error_msg + strlen(error_msg),
				"cellhd.proj = %d (lat/long)\n", cellhd.proj);
		    else if (cellhd.proj == PROJECTION_UTM)
			sprintf(error_msg + strlen(error_msg),
				"cellhd.proj = %d (UTM), zone = %d\n",
				cellhd.proj, cellhd.zone);
		    else
			sprintf(error_msg + strlen(error_msg),
				"cellhd.proj = %d (unknown), zone = %d\n",
				cellhd.proj, cellhd.zone);
		}
		strcat(error_msg,
		       _("\nIn case of no significant differences in the projection definitions,"
			 " use the -o flag to ignore them and use"
			 " current location definition.\n"));
		strcat(error_msg,
		       _("Consider generating a new location from the input dataset using "
			"the 'location' parameter.\n"));

                if (flag_j->answer)
                    msg_fn = G_message;
                else
                    msg_fn = G_fatal_error;
                msg_fn(error_msg);
                if (flag_j->answer)
                    exit(EXIT_FAILURE);
	    }
	    else {
                if (flag_j->answer)
                    msg_fn = G_message;
                else
                    msg_fn = G_verbose_message;            
                msg_fn(_("Projection of input dataset and current location "
                         "appear to match"));
                if (flag_j->answer)
                    exit(EXIT_SUCCESS);
	    }
	}
    }

    G_message(_("Proceeding with import of %d raster bands..."),
              GDALGetRasterCount(hDS));

    /* -------------------------------------------------------------------- */
    /*      Set the active window to match the available data.              */
    /* -------------------------------------------------------------------- */
    Rast_set_window(&cellhd);

    /* -------------------------------------------------------------------- */
    /*      Do we want to generate a simple raster, or an imagery group?    */
    /* -------------------------------------------------------------------- */
    n_bands = 0;
    if (parm.band->answer != NULL) {
	while (parm.band->answers[n_bands])
	    n_bands++;
    }

    if ((GDALGetRasterCount(hDS) > 1 && n_bands != 1)
	|| GDALGetGCPCount(hDS) > 0)
	force_imagery = TRUE;

    /* -------------------------------------------------------------------- */
    /*      Simple case.  Import a single band as a raster cell.            */
    /* -------------------------------------------------------------------- */
    if (!force_imagery) {
	int nBand = 1;

	if (parm.band->answer != NULL)
	    nBand = atoi(parm.band->answers[0]);

	hBand = GDALGetRasterBand(hDS, nBand);
	if (hBand == NULL) {
	    G_fatal_error(_("Selected band (%d) does not exist"), nBand);
	}

	ImportBand(hBand, output, NULL);
	if (parm.rat->answer)
	    dump_rat(hBand, parm.rat->answer, nBand);

	if (title)
	    Rast_put_cell_title(output, title);
    }

    /* -------------------------------------------------------------------- */
    /*      Complete case, import a set of raster bands as an imagery       */
    /*      group.                                                          */
    /* -------------------------------------------------------------------- */
    else {
	struct Ref ref;
	char szBandName[512];
	int nBand = 0;
	char colornamebuf[512], colornamebuf2[512];
	FILE *map_names_file = NULL;

	if(parm.map_names_file->answer) {
	    map_names_file = fopen(parm.map_names_file->answer, "w");
	    if(map_names_file == NULL) {
	        G_fatal_error(_("Unable to open the map names output text file"));
	    }
	}

	I_init_group_ref(&ref);

	colornamebuf2[0] = '\0';

	n_bands = 0;
	while (TRUE) {
	    if (parm.band->answer != NULL) {
		if (parm.band->answers[n_bands] == NULL)
		    break;
		nBand = atoi(parm.band->answers[n_bands++]);
	    }
	    else {
		if (nBand >= GDALGetRasterCount(hDS))
		    break;
		nBand++;
	    }

            /* Generate the suffix */
            if(num_digits > 0) {
                G_snprintf(suffix, num_digits + 1, "%0*d", num_digits, nBand + offset);
            } else {
                G_snprintf(suffix, 65, "%d", nBand + offset);
            }

	    G_debug(3, "Import raster band %d", nBand);
	    hBand = GDALGetRasterBand(hDS, nBand);
	    if (!hBand)
		G_fatal_error(_("Unable to get raster band number %d"), nBand);
	    if (!flag_k->answer) {
		/* use channel color names if present: */
		strcpy(colornamebuf,
		       GDALGetColorInterpretationName
		       (GDALGetRasterColorInterpretation(hBand)));

		/* check: two channels with identical name ? */
		if (strcmp(colornamebuf, colornamebuf2) == 0)
		    sprintf(colornamebuf, "%s", suffix);
		else
		    strcpy(colornamebuf2, colornamebuf);

		/* avoid bad color names; in case of 'Gray' often all channels are named 'Gray' */
		if (strcmp(colornamebuf, "Undefined") == 0 ||
		    strcmp(colornamebuf, "Gray") == 0)
		    sprintf(szBandName, "%s.%s", output, suffix);
		else {
		    G_tolcase(colornamebuf);
		    sprintf(szBandName, "%s.%s", output, colornamebuf);
		}
	    }
	    else
		sprintf(szBandName, "%s.%s", output, suffix);

            if (!parm.outloc->answer) {	/* Check if the map exists */
              if (G_find_raster2(szBandName, G_mapset())) {
                if (overwrite)
                  G_warning(_("Raster map <%s> already exists and will be overwritten"),
                            szBandName);
                else
                  G_fatal_error(_("Raster map <%s> already exists"), szBandName);
              }
            }

	    ImportBand(hBand, szBandName, &ref);

	    if(map_names_file)
	        fprintf(map_names_file, "%s\n", szBandName);

	    if (title)
		Rast_put_cell_title(szBandName, title);
	}

	if(map_names_file)
	    fclose(map_names_file);

	I_put_group_ref(output, &ref);
	I_free_group_ref(&ref);

	/* make this group the current group */
	I_put_group(output);

	/* -------------------------------------------------------------------- */
	/*      Output GCPs if present, we can only do this when writing an     */
	/*      imagery group.                                                  */
	/* -------------------------------------------------------------------- */
	if (GDALGetGCPCount(hDS) > 0) {
	    struct Control_Points sPoints;
	    const GDAL_GCP *pasGCPs = GDALGetGCPs(hDS);
	    int iGCP;
	    struct pj_info iproj,	/* input map proj parameters    */
	      oproj;		/* output map proj parameters   */
	    int create_target;
	    struct Cell_head gcpcellhd;
	    double emin, emax, nmin, nmax;
	    
	    G_zero(&gcpcellhd, sizeof(struct Cell_head));

	    sPoints.count = GDALGetGCPCount(hDS);
	    sPoints.e1 =
		(double *)G_malloc(sizeof(double) * sPoints.count * 4);
	    sPoints.n1 = sPoints.e1 + sPoints.count;
	    sPoints.e2 = sPoints.e1 + 2 * sPoints.count;
	    sPoints.n2 = sPoints.e1 + 3 * sPoints.count;
	    sPoints.status = (int *)G_malloc(sizeof(int) * sPoints.count);

	    G_message(_("Copying %d GCPS in points file for <%s>"),
		      sPoints.count, output);
	    if (GDALGetGCPProjection(hDS) != NULL
		&& strlen(GDALGetGCPProjection(hDS)) > 0) {
		G_message("%s\n"
		          "--------------------------------------------\n"
			  "%s\n"
			  "--------------------------------------------",
			  _("GCPs have the following OpenGIS WKT Coordinate System:"),
			  GDALGetGCPProjection(hDS));
	    }

	    create_target = 0;
	    if (parm.target->answer) {
		char target_mapset[GMAPSET_MAX];
		
		/* does the target location exist? */
		G_create_alt_env();
		G_setenv_nogisrc("LOCATION_NAME", parm.target->answer);
		sprintf(target_mapset, "PERMANENT");	/* must exist */

		if (G_mapset_permissions(target_mapset) == -1) {
		    /* create target location later */
		    create_target = 1;
		}
		G_switch_env();
	    }

	    if (parm.target->answer && !create_target) {
		SetupReprojector(GDALGetGCPProjection(hDS),
				 parm.target->answer, &iproj, &oproj);
		G_message(_("Re-projecting GCPs table:"));
		G_message(_("* Input projection for GCP table: %s"),
			  iproj.proj);
		G_message(_("* Output projection for GCP table: %s"),
			  oproj.proj);
	    }

	    emin = emax = pasGCPs[0].dfGCPX;
	    nmin = nmax = pasGCPs[0].dfGCPY;

	    for (iGCP = 0; iGCP < sPoints.count; iGCP++) {
		sPoints.e1[iGCP] = pasGCPs[iGCP].dfGCPPixel;
		/* GDAL lines from N to S -> GRASS Y from S to N */
		sPoints.n1[iGCP] = cellhd.rows - pasGCPs[iGCP].dfGCPLine;

		sPoints.e2[iGCP] = pasGCPs[iGCP].dfGCPX;	/* target */
		sPoints.n2[iGCP] = pasGCPs[iGCP].dfGCPY;
		sPoints.status[iGCP] = 1;

		/* If desired, do GCPs transformation to other projection */
		if (parm.target->answer) {
		    /* re-project target GCPs */
		    if (pj_do_proj(&(sPoints.e2[iGCP]), &(sPoints.n2[iGCP]),
				   &iproj, &oproj) < 0)
			G_fatal_error(_("Error in pj_do_proj (can't "
					"re-projection GCP %i)"), iGCP);
		}

		/* figure out legal e, w, n, s values for new target location */
		if (create_target) {
		    if (emin > pasGCPs[iGCP].dfGCPX)
			emin = pasGCPs[iGCP].dfGCPX;
		    if (emax < pasGCPs[iGCP].dfGCPX)
			emax = pasGCPs[iGCP].dfGCPX;
		    if (nmin > pasGCPs[iGCP].dfGCPY)
			nmin = pasGCPs[iGCP].dfGCPY;
		    if (nmax < pasGCPs[iGCP].dfGCPY)
			nmax = pasGCPs[iGCP].dfGCPY;
		}
	    }			/* for all GCPs */

	    I_put_control_points(output, &sPoints);
	    if (create_target) {
		/* create target location */
		if (GPJ_wkt_to_grass(&gcpcellhd, &proj_info,
				     &proj_units, GDALGetGCPProjection(hDS), 0) < 0) {
		    G_warning(_("Unable to convert input map projection to GRASS "
				    "format; cannot create new location."));
		}
		else {
		    gcpcellhd.west = emin;
		    gcpcellhd.east = emax;
		    gcpcellhd.south = nmin;
		    gcpcellhd.north = nmax;
		    gcpcellhd.rows = GDALGetRasterYSize(hDS);
		    gcpcellhd.cols = GDALGetRasterXSize(hDS);
		    gcpcellhd.ns_res = 1.0;
		    gcpcellhd.ns_res3 = 1.0;
		    gcpcellhd.ew_res = 1.0;
		    gcpcellhd.ew_res3 = 1.0;
		    gcpcellhd.top = 1.;
		    gcpcellhd.bottom = 0.;
		    gcpcellhd.tb_res = 1.;
		    gcpcellhd.depths = 1;
		    
		    G_adjust_Cell_head(&gcpcellhd, 1, 1);

		    G_create_alt_env();
		    if (0 != G_make_location(parm.target->answer, &gcpcellhd,
					     proj_info, proj_units)) {
			G_fatal_error(_("Unable to create new location <%s>"),
				      parm.target->answer);
		    }
		    /* switch back to import location */
		    G_switch_env();

		    G_message(_("Location <%s> created"), parm.target->answer);
		    /* set the group's target */
		    I_put_target(output, parm.target->answer, "PERMANENT");
		    G_message(_("The target for the output group <%s> has been set to "
				"location <%s>, mapset <PERMANENT>."),
				output, parm.target->answer);
		}
	    }

	    G_free(sPoints.e1);
	    G_free(sPoints.status);
	}
    }

    /* close the GDALDataset to avoid segfault in libgdal */
    GDALClose(hDS);

    /* -------------------------------------------------------------------- */
    /*      Extend current window based on dataset.                         */
    /* -------------------------------------------------------------------- */
    if (flag_e->answer) {
	if (strcmp(G_mapset(), "PERMANENT") == 0)
	    /* fixme: expand WIND and DEFAULT_WIND independently. (currently
		WIND gets forgotten and DEFAULT_WIND is expanded for both) */
	    G_get_default_window(&cur_wind);
	else
	    G_get_window(&cur_wind);

	cur_wind.north = MAX(cur_wind.north, cellhd.north);
	cur_wind.south = MIN(cur_wind.south, cellhd.south);
	cur_wind.west = MIN(cur_wind.west, cellhd.west);
	cur_wind.east = MAX(cur_wind.east, cellhd.east);

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

    exit(EXIT_SUCCESS);
}

/************************************************************************/
/*                          SetupReprojector()                          */

/************************************************************************/

static void SetupReprojector(const char *pszSrcWKT, const char *pszDstLoc,
			     struct pj_info *iproj, struct pj_info *oproj)
{
    struct Cell_head cellhd;
    struct Key_Value *proj_info = NULL, *proj_units = NULL;
    char errbuf[256];
    int permissions;
    char target_mapset[GMAPSET_MAX];
    struct Key_Value *out_proj_info,	/* projection information of    */
     *out_unit_info;		/* input and output mapsets     */

    /* -------------------------------------------------------------------- */
    /*      Translate GCP WKT coordinate system into GRASS format.          */
    /* -------------------------------------------------------------------- */
    GPJ_wkt_to_grass(&cellhd, &proj_info, &proj_units, pszSrcWKT, 0);

    if (pj_get_kv(iproj, proj_info, proj_units) < 0)
	G_fatal_error(_("Unable to translate projection key values of input GCPs"));

    /* -------------------------------------------------------------------- */
    /*      Get the projection of the target location.                      */
    /* -------------------------------------------------------------------- */

    /* Change to user defined target location for GCPs transformation */
    G_create_alt_env();
    G_setenv_nogisrc("LOCATION_NAME", (char *)pszDstLoc);
    sprintf(target_mapset, "PERMANENT");	/* to find PROJ_INFO */

    permissions = G_mapset_permissions(target_mapset);
    if (permissions >= 0) {

	/* Get projection info from target location */
	if ((out_proj_info = G_get_projinfo()) == NULL)
	    G_fatal_error(_("Unable to get projection info of target location"));
	if ((out_unit_info = G_get_projunits()) == NULL)
	    G_fatal_error(_("Unable to get projection units of target location"));
	if (pj_get_kv(oproj, out_proj_info, out_unit_info) < 0)
	    G_fatal_error(_("Unable to get projection key values of target location"));
    }
    else {			/* can't access target mapset */
	/* access to mapset PERMANENT in target location is not required */
	sprintf(errbuf, _("Mapset <%s> in target location <%s> - "),
		target_mapset, pszDstLoc);
	strcat(errbuf, permissions == 0 ? _("permission denied")
	       : _("not found"));
	G_fatal_error("%s", errbuf);
    }				/* permission check */

    /* And switch back to original location */
    G_switch_env();
}


/************************************************************************/
/*                             ImportBand()                             */

/************************************************************************/

static void ImportBand(GDALRasterBandH hBand, const char *output,
		       struct Ref *group_ref)
{
    RASTER_MAP_TYPE data_type;
    GDALDataType eGDT, eRawGDT;
    int row, nrows, ncols, complex;
    int cf, cfR, cfI, bNoDataEnabled;
    int indx;
    void *cell, *cellReal, *cellImg;
    void *bufComplex;
    double dfNoData;
    char outputReal[GNAME_MAX], outputImg[GNAME_MAX];
    char *nullFlags = NULL;
    struct History history;
    char **GDALmetadata;
    int have_colors = 0;
    GDALRasterAttributeTableH gdal_rat;

    G_message(_("Importing raster map <%s>..."), output);
    
    /* -------------------------------------------------------------------- */
    /*      Select a cell type for the new cell.                            */
    /* -------------------------------------------------------------------- */
    eRawGDT = GDALGetRasterDataType(hBand);

    switch (eRawGDT) {
    case GDT_Float32:
	data_type = FCELL_TYPE;
	eGDT = GDT_Float32;
	complex = FALSE;
	break;

    case GDT_Float64:
	data_type = DCELL_TYPE;
	eGDT = GDT_Float64;
	complex = FALSE;
	break;

    case GDT_Byte:
	data_type = CELL_TYPE;
	eGDT = GDT_Int32;
	complex = FALSE;
	Rast_set_cell_format(0);
	break;

    case GDT_Int16:
    case GDT_UInt16:
	data_type = CELL_TYPE;
	eGDT = GDT_Int32;
	complex = FALSE;
	Rast_set_cell_format(1);
	break;

    default:
	data_type = CELL_TYPE;
	eGDT = GDT_Int32;
	complex = FALSE;
	Rast_set_cell_format(3);
	break;
    }

    /* -------------------------------------------------------------------- */
    /*      Create the new raster(s)                                          */
    /* -------------------------------------------------------------------- */
    ncols = GDALGetRasterBandXSize(hBand);
    nrows = GDALGetRasterBandYSize(hBand);

    if (complex) {
	sprintf(outputReal, "%s.real", output);
	cfR = Rast_open_new(outputReal, data_type);
	sprintf(outputImg, "%s.imaginary", output);

	cfI = Rast_open_new(outputImg, data_type);

	cellReal = Rast_allocate_buf(data_type);
	cellImg = Rast_allocate_buf(data_type);
	if (eGDT == GDT_Float64)
	    bufComplex = (double *)G_malloc(sizeof(double) * ncols * 2);
	else
	    bufComplex = (float *)G_malloc(sizeof(float) * ncols * 2);

	if (group_ref != NULL) {
	    I_add_file_to_group_ref(outputReal, G_mapset(), group_ref);
	    I_add_file_to_group_ref(outputImg, G_mapset(), group_ref);
	}
    }
    else {
	cf = Rast_open_new(output, data_type);

	if (group_ref != NULL)
	    I_add_file_to_group_ref((char *)output, G_mapset(), group_ref);

	cell = Rast_allocate_buf(data_type);
    }

    /* -------------------------------------------------------------------- */
    /*      Do we have a null value?                                        */
    /* -------------------------------------------------------------------- */
    dfNoData = GDALGetRasterNoDataValue(hBand, &bNoDataEnabled);
    if (bNoDataEnabled) {
	nullFlags = (char *)G_malloc(sizeof(char) * ncols);
	memset(nullFlags, 0, ncols);
    }

    /* -------------------------------------------------------------------- */
    /*      Write the raster one scanline at a time.                        */
    /*      We have to distinguish some cases due to the different          */
    /*      coordinate system orientation of GDAL and GRASS for xy data     */
    /* -------------------------------------------------------------------- */
    if (!l1bdriver) {		/* no AVHRR */
	for (row = 1; row <= nrows; row++) {
	    if (complex) {	/* CEOS SAR et al.: import flipped to match GRASS coordinates */
		GDALRasterIO(hBand, GF_Read, 0, row - 1, ncols, 1,
			     bufComplex, ncols, 1, eGDT, 0, 0);

		for (indx = ncols - 1; indx >= 0; indx--) {	/* CEOS: flip east-west during import - MN */
		    if (eGDT == GDT_Int32) {
			((CELL *) cellReal)[ncols - indx] =
			    ((GInt32 *) bufComplex)[indx * 2];
			((CELL *) cellImg)[ncols - indx] =
			    ((GInt32 *) bufComplex)[indx * 2 + 1];
		    }
		    else if (eGDT == GDT_Float32) {
			((FCELL *)cellReal)[ncols - indx] =
			    ((float *)bufComplex)[indx * 2];
			((FCELL *)cellImg)[ncols - indx] =
			    ((float *)bufComplex)[indx * 2 + 1];
		    }
		    else if (eGDT == GDT_Float64) {
			((DCELL *)cellReal)[ncols - indx] =
			    ((double *)bufComplex)[indx * 2];
			((DCELL *)cellImg)[ncols - indx] =
			    ((double *)bufComplex)[indx * 2 + 1];
		    }
		}
		Rast_put_row(cfR, cellReal, data_type);
		Rast_put_row(cfI, cellImg, data_type);
	    }			/* end of complex */
	    else {		/* single band */
		GDALRasterIO(hBand, GF_Read, 0, row - 1, ncols, 1,
			     cell, ncols, 1, eGDT, 0, 0);

		if (nullFlags != NULL) {
		    memset(nullFlags, 0, ncols);

		    if (eGDT == GDT_Int32) {
			for (indx = 0; indx < ncols; indx++) {
			    if (((CELL *) cell)[indx] == (GInt32) dfNoData) {
				nullFlags[indx] = 1;
			    }
			}
		    }
		    else if (eGDT == GDT_Float32) {
			for (indx = 0; indx < ncols; indx++) {
			    if (((FCELL *)cell)[indx] == (float)dfNoData) {
				nullFlags[indx] = 1;
			    }
			}
		    }
		    else if (eGDT == GDT_Float64) {
			for (indx = 0; indx < ncols; indx++) {
			    if (((DCELL *)cell)[indx] == dfNoData) {
				nullFlags[indx] = 1;
			    }
			}
		    }

		    Rast_insert_null_values(cell, nullFlags, ncols, data_type);
		}

		Rast_put_row(cf, cell, data_type);
	    }			/* end of not complex */

	    G_percent(row - 1, nrows, 2);
	}			/* for loop */
    }				/* end of not AVHRR */
    else {
	/* AVHRR - read from south to north to match GCPs */
	/* AVHRR - as for other formats, read from north to south to match GCPs 
	 * MM 2013 with gdal 1.10 */
	for (row = 1; row <= nrows; row++) {
	    GDALRasterIO(hBand, GF_Read, 0, row - 1, ncols, 1,
			 cell, ncols, 1, eGDT, 0, 0);

	    if (nullFlags != NULL) {
		memset(nullFlags, 0, ncols);

		if (eGDT == GDT_Int32) {
		    for (indx = 0; indx < ncols; indx++) {
			if (((CELL *) cell)[indx] == (CELL) dfNoData) {
			    nullFlags[indx] = 1;
			}
		    }
		}
		else if (eGDT == GDT_Float32) {
		    for (indx = 0; indx < ncols; indx++) {
			if (((FCELL *)cell)[indx] == (FCELL)dfNoData) {
			    nullFlags[indx] = 1;
			}
		    }
		}
		else if (eGDT == GDT_Float64) {
		    for (indx = 0; indx < ncols; indx++) {
			if (((DCELL *)cell)[indx] == dfNoData) {
			    nullFlags[indx] = 1;
			}
		    }
		}

		Rast_insert_null_values(cell, nullFlags, ncols, data_type);
	    }

	    Rast_put_row(cf, cell, data_type);
	}

	G_percent(row, nrows, 2);
    }				/* end AVHRR */
    G_percent(1, 1, 1);

    /* -------------------------------------------------------------------- */
    /*      Cleanup                                                         */
    /* -------------------------------------------------------------------- */
    if (complex) {
	G_debug(1, "Creating support files for %s", outputReal);
	Rast_close(cfR);
	Rast_short_history((char *)outputReal, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history((char *)outputReal, &history);

	G_debug(1, "Creating support files for %s", outputImg);
	Rast_close(cfI);
	Rast_short_history((char *)outputImg, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history((char *)outputImg, &history);

	G_free(bufComplex);
	G_free(cellReal);
	G_free(cellImg);
    }
    else {
	G_debug(1, "Creating support files for %s", output);
	Rast_close(cf);
	Rast_short_history((char *)output, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history((char *)output, &history);

	G_free(cell);
    }

    if (nullFlags != NULL)
	G_free(nullFlags);

    /* -------------------------------------------------------------------- */
    /*      Transfer colormap, if there is one.                             */
    /*      prefer color rules over color tables, search:                   */
    /*      1. GRASS color rules in metadata                                */
    /*      2. Raster attribute table with color rules                      */
    /*      3. Raster color table                                           */
    /* -------------------------------------------------------------------- */

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
    
    /* categories in raster attribute table? */
    
    if (gdal_rat != NULL) {
	nrows = GDALRATGetRowCount(gdal_rat);
	ncols = GDALRATGetColumnCount(gdal_rat);
	
	if (nrows > 0 && ncols > 0) {
	    int minc, maxc, minmaxc, namec;
	    GDALRATFieldUsage field_use;
	    DCELL val1, val2;
	    struct Categories cats;
	    const char *label;

	    minc = maxc = minmaxc = namec = -1;
	    for (indx = 0; indx < ncols; indx++) {
		 field_use = GDALRATGetUsageOfCol(gdal_rat, indx);
		 
		 if (field_use == GFU_Min)
		    minc = indx;
		 else if (field_use == GFU_Max)
		    maxc = indx;
		 else if (field_use == GFU_MinMax)
		    minmaxc = indx;
		 else if (field_use == GFU_Name)
		    namec = indx;
	    }

	    if (namec >= 0 && minmaxc >= 0) {
		Rast_init_cats("", &cats);

		/* fetch labels */
		for (indx = 0; indx < nrows; indx++) {
		    val1 = GDALRATGetValueAsDouble(gdal_rat, indx, minmaxc);
		    val2 = val1;
		    label = GDALRATGetValueAsString(gdal_rat, indx, namec);
		    
		    if (label)
			Rast_set_d_cat(&val1, &val2, label, &cats);
		}
		Rast_write_cats(output, &cats);

		Rast_free_cats(&cats);
	    }
	    else if (namec >= 0 && minc >= 0 && maxc >= 0) {
		Rast_init_cats("", &cats);

		/* fetch labels */
		for (indx = 0; indx < nrows; indx++) {
		    val1 = GDALRATGetValueAsDouble(gdal_rat, indx, minc);
		    val2 = GDALRATGetValueAsDouble(gdal_rat, indx, maxc);
		    label = GDALRATGetValueAsString(gdal_rat, indx, namec);
		    
		    if (label)
			Rast_set_d_cat(&val1, &val2, label, &cats);
		}
		Rast_write_cats(output, &cats);

		Rast_free_cats(&cats);
	    }
	}
    }

    return;
}

static int dump_rat(GDALRasterBandH hBand, char *outrat, int nBand)
{
    int row, col, nrows, ncols;
    const char *field_name;
    GDALRATFieldUsage field_use;
    GDALRATFieldType *field_type;
    GDALRasterAttributeTableH gdal_rat;
    FILE *fp;
    char fname[GNAME_MAX];
    
    if ((gdal_rat = GDALGetDefaultRAT(hBand)) == NULL)
	return 0;

    nrows = GDALRATGetRowCount(gdal_rat);
    ncols = GDALRATGetColumnCount(gdal_rat);

    if (nrows == 0 || ncols == 0)
	return 0;

    field_type = G_malloc(ncols * sizeof(GDALRATFieldType));
    
    G_snprintf(fname, GNAME_MAX, "%s_%d.csv", outrat, nBand);
    if (!(fp = fopen(fname, "w"))) {
	int err = errno;
	
	G_fatal_error(_("Unable to open file <%s>: %s."),
	              fname, strerror(err));
    }

    /* dump column names and usage */
    for (col = 0; col < ncols; col++) {
	
	if (col)
	    fprintf(fp, "|");
	
	field_name = GDALRATGetNameOfCol(gdal_rat, col);
	fprintf(fp, "%s", field_name);
	field_use = GDALRATGetUsageOfCol(gdal_rat, col);
	
	if (field_use == GFU_Generic)
	    fprintf(fp, " (General purpose field)");
	else if (field_use == GFU_PixelCount)
	    fprintf(fp, " (Histogram pixel count)");
	else if (field_use == GFU_Name)
	    fprintf(fp, " (Class name)");
	else if (field_use == GFU_Min)
	    fprintf(fp, " (Class range minimum)");
	else if (field_use == GFU_Max)
	    fprintf(fp, " (Class range maximum)");
	else if (field_use == GFU_MinMax)
	    fprintf(fp, " (Class value (min=max))");
	else if (field_use == GFU_Red)
	    fprintf(fp, " (Red class color (0-255))");
	else if (field_use == GFU_Green)
	    fprintf(fp, " (Green class color (0-255))");
	else if (field_use == GFU_Blue)
	    fprintf(fp, " (Blue class color (0-255))");
	else if (field_use == GFU_Alpha)
	    fprintf(fp, " (Alpha (0=transparent,255=opaque))");
	else if (field_use == GFU_RedMin)
	    fprintf(fp, " (Color Range Red Minimum)");
	else if (field_use == GFU_GreenMin)
	    fprintf(fp, " (Color Range Green Minimum)");
	else if (field_use == GFU_BlueMin)
	    fprintf(fp, " (Color Range Blue Minimum)");
	else if (field_use == GFU_AlphaMin)
	    fprintf(fp, " (Color Range Alpha Minimum)");
	else if (field_use == GFU_RedMax)
	    fprintf(fp, " (Color Range Red Maximum)");
	else if (field_use == GFU_GreenMax)
	    fprintf(fp, " (Color Range Green Maximum)");
	else if (field_use == GFU_BlueMax)
	    fprintf(fp, " (Color Range Blue Maximum)");
	else if (field_use == GFU_AlphaMax)
	    fprintf(fp, " (Color Range Alpha Maximum)");
	else if (field_use == GFU_MaxCount)
	    fprintf(fp, " (Maximum GFU value)");
	else
	    fprintf(fp, " (Unknown)");

	/* remember column type */
	field_type[col] = GDALRATGetTypeOfCol(gdal_rat, col);
    }
    fprintf(fp, "\n");
    
    /* dump entries */
    for (row = 0; row < nrows; row++) {

	for (col = 0; col < ncols; col++) {
	    if (col)
		fprintf(fp, "|");
	    if (field_type[col] == GFT_Integer)
		fprintf(fp, "%d", GDALRATGetValueAsInt(gdal_rat, row, col));
	    else if (field_type[col] == GFT_Real)
		fprintf(fp, "%.15g", GDALRATGetValueAsDouble(gdal_rat, row, col));
	    else
		fprintf(fp, "%s", GDALRATGetValueAsString(gdal_rat, row, col));
	}
	fprintf(fp, "\n");
    }
    fclose(fp);

    return 1;
}

void error_handler_ds(void *p)
{
    GDALDatasetH hDS = (GDALDatasetH) p;
    GDALClose(hDS);
}
