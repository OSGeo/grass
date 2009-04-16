
/****************************************************************************
*
* MODULE:       r.out.gdal
* AUTHOR(S):    Vytautas Vebra <olivership@gmail.com>
* PURPOSE:      Exports GRASS raster to GDAL suported formats;
*               based on GDAL library.
*               Replaces r.out.gdal.sh script which used the gdal_translate
*               executable and GDAL grass-format plugin.
* COPYRIGHT:    (C) 2006-2008 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/

/* Undefine this if you do not want any extra funtion calls before G_parse() */
#define __ALLOW_DYNAMIC_OPTIONS__

#include <stdlib.h>
#include <unistd.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/imagery.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>
#include <grass/dbmi.h>

#include "cpl_string.h"
#include "local_proto.h"


void supported_formats(const char **formats)
{
    /* Code taken from r.in.gdal */

    int iDr;

    dbString gdal_formats;

    db_init_string(&gdal_formats);

    if (*formats)
	fprintf(stdout, _("Supported formats:\n"));

    for (iDr = 0; iDr < GDALGetDriverCount(); iDr++) {

	GDALDriverH hDriver = GDALGetDriver(iDr);
	const char *pszRWFlag;

	if (GDALGetMetadataItem(hDriver, GDAL_DCAP_CREATE, NULL))
	    pszRWFlag = "rw+";
	else if (GDALGetMetadataItem(hDriver, GDAL_DCAP_CREATECOPY, NULL))
	    pszRWFlag = "rw";
	else
	    pszRWFlag = "ro";

	if (*formats)
	    fprintf(stdout, "  %s (%s): %s\n",
		    GDALGetDriverShortName(hDriver), pszRWFlag,
		    GDALGetDriverLongName(hDriver));
	else {
	    if (GDALGetMetadataItem(hDriver, GDAL_DCAP_CREATE, NULL) ||
		GDALGetMetadataItem(hDriver, GDAL_DCAP_CREATECOPY, NULL)) {
		if (db_sizeof_string(&gdal_formats) > 0)
		    db_append_string(&gdal_formats, ",");

		db_append_string(&gdal_formats,
				 (char *)GDALGetDriverShortName(hDriver));
	    }
	}
    }

    if (db_sizeof_string(&gdal_formats) > 0)
	*formats = G_store(db_get_string(&gdal_formats));

    return;
}


static void AttachMetadata(GDALDatasetH hDS, char **papszMetadataOptions)
/* function taken from gdal_translate */
{
    int nCount = CSLCount(papszMetadataOptions);
    int i;

    for (i = 0; i < nCount; i++) {
	char *pszKey = NULL;
	const char *pszValue;

	pszValue = CPLParseNameValue(papszMetadataOptions[i], &pszKey);
	GDALSetMetadataItem(hDS, pszKey, pszValue, NULL);
	CPLFree(pszKey);
    }

    CSLDestroy(papszMetadataOptions);
}


int main(int argc, char *argv[])
{

    struct GModule *module;
    struct Flag *flag_l, *flag_c;
    struct Option *input, *format, *type, *output, *createopt, *metaopt,
	*nodataopt;

    struct Cell_head cellhead;
    struct Ref ref;
    const char *mapset, *gdal_formats = NULL;
    RASTER_MAP_TYPE maptype;
    int bHaveMinMax;
    double dfCellMin;
    double dfCellMax;
    struct FPRange sRange;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->description =
	_("Exports GRASS raster maps into GDAL supported formats.");
    module->keywords = _("raster, export");

    flag_l = G_define_flag();
    flag_l->key = 'l';
    flag_l->description = _("List supported output formats");
    flag_l->guisection = _("Print");

    flag_c = G_define_flag();
    flag_c->key = 'c';
    flag_c->label = _("Do not write GDAL standard colortable");
    flag_c->description = _("Only applicable to Byte or UInt16 data types.");

    input = G_define_standard_option(G_OPT_R_INPUT);
    input->required = NO;
    input->description = _("Name of raster map (or group) to export");
    input->guisection = _("Required");

    format = G_define_option();
    format->key = "format";
    format->type = TYPE_STRING;
    format->description =
	_("GIS format to write (case sensitive, see also -l flag)");

#ifdef __ALLOW_DYNAMIC_OPTIONS__
    /* Init GDAL */
    GDALAllRegister();
    supported_formats(&gdal_formats);
    if (gdal_formats)
	format->options = G_store(gdal_formats);
    /* else
     * G_fatal_error (_("Unknown GIS formats")); */
#else
    gdal_formats =
	"AAIGrid,BMP,BSB,DTED,ELAS,ENVI,FIT,GIF,GTiff,HFA,JPEG,MEM,MFF,MFF2,NITF,PAux,PNG,PNM,VRT,XPM";
    format->options = gdal_formats;
#endif
    format->answer = "GTiff";
    format->required = NO;

    type = G_define_option();
    type->key = "type";
    type->type = TYPE_STRING;
    type->description = _("File type");
    type->options =
	"Byte,Int16,UInt16,Int32,UInt32,Float32,Float64,CInt16,CInt32,CFloat32,CFloat64";
    type->required = NO;

    output = G_define_standard_option(G_OPT_R_OUTPUT);
    output->required = NO;
    output->gisprompt = "new_file,file,output";
    output->description = _("Name for output raster file");
    output->guisection = _("Required");

    createopt = G_define_option();
    createopt->key = "createopt";
    createopt->type = TYPE_STRING;
    createopt->label =
	_("Creation option(s) to pass to the output format driver");
    createopt->description =
	_("In the form of \"NAME=VALUE\", separate multiple entries with a comma.");
    createopt->multiple = YES;
    createopt->required = NO;

    metaopt = G_define_option();
    metaopt->key = "metaopt";
    metaopt->type = TYPE_STRING;
    metaopt->label = _("Metadata key(s) and value(s) to include");
    metaopt->description =
	_("In the form of \"META-TAG=VALUE\", separate multiple entries "
	  "with a comma. Not supported by all output format drivers.");
    metaopt->multiple = YES;
    metaopt->required = NO;

    nodataopt = G_define_option();
    nodataopt->key = "nodata";
    nodataopt->type = TYPE_DOUBLE;
    nodataopt->description =
	_("Assign a specified nodata value to output bands");
    nodataopt->multiple = NO;
    nodataopt->required = NO;


    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


#ifndef __ALLOW_DYNAMIC_OPTIONS__
    /* Init GDAL */
    GDALAllRegister();
#endif

    if (flag_l->answer) {
	supported_formats(&gdal_formats);
	exit(EXIT_SUCCESS);
    }

    if (!input->answer)
	G_fatal_error(_("Required parameter <%s> not set"), input->key);

    /* Try to open input GRASS raster.. */
    mapset = G_find_cell2(input->answer, "");

    if (mapset != NULL) {
	/* Add input to "group". "Group" whith 1 raster (band) will exist only in memory. */
	I_init_group_ref(&ref);
	I_add_file_to_group_ref(input->answer, mapset, &ref);
    }
    else {
	/* Maybe input is group. Try to read group file */
	if (I_get_group_ref(input->answer, &ref) != 1)
	    G_fatal_error(_("Raster map or group <%s> not found"),
			  input->answer);
    }
    if (ref.nfiles == 0)
	G_fatal_error(_("No raster maps in group <%s>"), input->answer);

    /* Read project and region data */
    struct Key_Value *projinfo = G_get_projinfo();
    struct Key_Value *projunits = G_get_projunits();
    char *srswkt = GPJ_grass_to_wkt(projinfo, projunits, 0, 0);

    G_get_window(&cellhead);

    /* Try to create raster data drivers. If failed - exit. */
    GDALDriverH hDriver = NULL, hMEMDriver = NULL;

    hDriver = GDALGetDriverByName(format->answer);
    if (hDriver == NULL)
	G_fatal_error(_("Unable to get <%s> driver"), format->answer);
    /* Does driver support GDALCreate ? */
    if (GDALGetMetadataItem(hDriver, GDAL_DCAP_CREATE, NULL) == NULL) {
	/* If not - create MEM driver for intermediate dataset. 
	 * Check if raster can be created at all (with GDALCreateCopy) */
	if (GDALGetMetadataItem(hDriver, GDAL_DCAP_CREATECOPY, NULL)) {
	    G_message(_("Driver <%s> does not support direct writing. "
			"Using MEM driver for intermediate dataset."),
		      format->answer);
	    hMEMDriver = GDALGetDriverByName("MEM");
	    if (hMEMDriver == NULL)
		G_fatal_error(_("Unable to get in-memory raster driver"));

	}
	else
	    G_fatal_error(_("Driver <%s> does not support creating rasters"),
			  format->answer);
    }

    /* Determine raster data type */
    GDALDataType datatype = GDT_Unknown;
    double nodataval;

    maptype = CELL_TYPE;

    if (type->answer) {
	/* reduce number of strcmps ... */
	if (type->answer[0] == 'B') {
	    datatype = GDT_Byte;
	    maptype = CELL_TYPE;
	    nodataval = TYPE_BYTE_MIN;
	}
	else if (type->answer[0] == 'I') {
	    if (strcmp(type->answer, "Int16") == 0) {
		datatype = GDT_Int16;
		maptype = CELL_TYPE;
		nodataval = TYPE_INT16_MIN;
	    }
	    else if (strcmp(type->answer, "Int32") == 0) {
		datatype = GDT_Int32;
		maptype = CELL_TYPE;
		nodataval = TYPE_INT32_MIN;
	    }
	}
	else if (type->answer[0] == 'U') {
	    if (strcmp(type->answer, "UInt16") == 0) {
		datatype = GDT_UInt16;
		maptype = CELL_TYPE;
		nodataval = TYPE_UINT16_MIN;
	    }
	    else if (strcmp(type->answer, "UInt32") == 0) {
		datatype = GDT_UInt32;
		maptype = DCELL_TYPE;
		nodataval = TYPE_UINT32_MIN;
	    }
	}
	else if (type->answer[0] == 'F') {
	    if (strcmp(type->answer, "Float32") == 0) {
		datatype = GDT_Float32;
		maptype = FCELL_TYPE;
		/* nodataval = TYPE_FLOAT32_MIN; */
		nodataval = 0.0/0.0;
	    }
	    else if (strcmp(type->answer, "Float64") == 0) {
		datatype = GDT_Float64;
		maptype = DCELL_TYPE;
		/* nodataval = TYPE_FLOAT64_MIN; */
		nodataval = 0.0/0.0;
	    }
	}
	else if (type->answer[0] == 'C') {
	    if (strcmp(type->answer, "CInt16") == 0) {
		datatype = GDT_CInt16;
		maptype = CELL_TYPE;
		nodataval = TYPE_INT16_MIN;
	    }
	    else if (strcmp(type->answer, "CInt32") == 0) {
		datatype = GDT_CInt32;
		maptype = CELL_TYPE;
		nodataval = TYPE_INT32_MIN;
	    }
	    else if (strcmp(type->answer, "CFloat32") == 0) {
		datatype = GDT_CFloat32;
		maptype = FCELL_TYPE;
		/* nodataval = TYPE_FLOAT32_MIN; */
		nodataval = 0.0/0.0;
	    }
	    else if (strcmp(type->answer, "CFloat64") == 0) {
		datatype = GDT_CFloat64;
		maptype = DCELL_TYPE;
		/* nodataval = TYPE_FLOAT64_MIN; */
		nodataval = 0.0/0.0;
	    }
	}
    }

    /* If file type not set by user ... */
    /* Get min/max values. */
    if (G_read_fp_range(ref.file[0].name, ref.file[0].mapset, &sRange) == -1) {
	bHaveMinMax = FALSE;
    }
    else {
	bHaveMinMax = TRUE;
	G_get_fp_range_min_max(&sRange, &dfCellMin, &dfCellMax);
    }
    G_debug(3, "Range: min: %f, max: %f", dfCellMin, dfCellMax);

    if (datatype == GDT_Unknown) {
	/* ... determine raster data type from first GRASS raster in a group */
	maptype = G_raster_map_type(ref.file[0].name, ref.file[0].mapset);
	if (maptype == FCELL_TYPE) {
	    datatype = GDT_Float32;
	    /* nodataval = TYPE_FLOAT32_MIN; */
	    nodataval = 0.0/0.0;
	}
	else if (maptype == DCELL_TYPE) {
	    datatype = GDT_Float64;
	    /* nodataval = TYPE_FLOAT64_MIN; */
	    nodataval = 0.0/0.0;
	}
	else {
	    /* Special tricks for GeoTIFF color table support and such */
	    if (dfCellMin >= 0 && dfCellMax < 256) {
		datatype = GDT_Byte;
		nodataval = TYPE_BYTE_MIN;
	    }
	    else {
		if (dfCellMin >= 0 && dfCellMax < 65536) {
		    datatype = GDT_UInt16;
		    nodataval = TYPE_UINT16_MIN;
		}
		else {
		    datatype = GDT_Int32;	/* need to fine tune this more? */
		    nodataval = TYPE_INT32_MIN;
		}
	    }
	}
    }

    /* default or user-specified nodata-value ? */
    int default_nodataval = 1;

    if (nodataopt->answer) {
	nodataval = atof(nodataopt->answer);
	default_nodataval = 0;
	/* check nodataval here, not in export_band(), because it is specified only once */
	if (nullvalue_check(nodataval, datatype))
	    G_fatal_error("Raster export aborted.");
    }

    G_debug(3, "Input map datatype=%s\n",
	    (maptype == CELL_TYPE ? "CELL" :
	     (maptype == DCELL_TYPE ? "DCELL" :
	      (maptype == FCELL_TYPE ? "FCELL" : "??"))));
    G_message(_("Exporting to GDAL data type: %s"),
	      GDALGetDataTypeName(datatype));

    /* Create dataset for output with target driver or, if needed, with in-memory driver */
    char **papszOptions = NULL;

    /* parse dataset creation options */
    if (createopt->answer) {
	int i;

	i = 0;
	while (createopt->answers[i]) {
	    papszOptions = CSLAddString(papszOptions, createopt->answers[i]);
	    i++;
	}
    }

    GDALDatasetH hCurrDS = NULL, hMEMDS = NULL, hDstDS = NULL;

    if (!output->answer)
	G_fatal_error(_("Output file name not specified"));
    if (hMEMDriver) {
	hMEMDS =
	    GDALCreate(hMEMDriver, "", cellhead.cols, cellhead.rows,
		       ref.nfiles, datatype, papszOptions);
	if (hMEMDS == NULL)
	    G_fatal_error(_("Unable to create dataset using "
			    "memory raster driver"));
	hCurrDS = hMEMDS;
    }
    else {
	hDstDS =
	    GDALCreate(hDriver, output->answer, cellhead.cols, cellhead.rows,
		       ref.nfiles, datatype, papszOptions);
	if (hDstDS == NULL)
	    G_fatal_error(_("Unable to create <%s> dataset using <%s> driver"),
			  output->answer, format->answer);
	hCurrDS = hDstDS;
    }

    /* Set Geo Transform  */
    double adfGeoTransform[6];

    adfGeoTransform[0] = cellhead.west;
    adfGeoTransform[1] = cellhead.ew_res;
    adfGeoTransform[2] = 0.0;
    adfGeoTransform[3] = cellhead.north;
    adfGeoTransform[4] = 0.0;
    adfGeoTransform[5] = -1 * cellhead.ns_res;
    if (GDALSetGeoTransform(hCurrDS, adfGeoTransform) >= CE_Failure)
	G_warning(_("Unable to set geo transform"));

    /* Set Projection  */
    CPLErr ret;

    if (srswkt)
	ret = GDALSetProjection(hCurrDS, srswkt);
    if (!srswkt || ret == CE_Failure)
	G_warning(_("Unable to set projection"));

    /* Add metadata */
    AttachMetadata(hCurrDS, metaopt->answers);

    /* Export to GDAL raster */
    int band;

    for (band = 0; band < ref.nfiles; band++) {
	if (ref.nfiles > 1) {
	    G_verbose_message(_("Exporting raster map <%s> (band %d)..."),
			      G_fully_qualified_name(ref.file[band].name,
						     ref.file[band].mapset),
			      band + 1);
	}
	/* do range check here because we don't have GDALDataType in export_band() */
	if (G_read_fp_range
	    (ref.file[band].name, ref.file[band].mapset, &sRange) == -1) {
	    bHaveMinMax = FALSE;
	}
	else {
	    bHaveMinMax = TRUE;
	    G_get_fp_range_min_max(&sRange, &dfCellMin, &dfCellMax);
	}
	G_debug(3, "Range: min: %f, max: %f", dfCellMin, dfCellMax);
	if (bHaveMinMax == TRUE) {
	    if (range_check
		(dfCellMin, dfCellMax, datatype, ref.file[band].name))
		G_fatal_error("Raster export aborted");
	}

	/* ready to export */
	if (export_band
	    (hCurrDS, band + 1, ref.file[band].name, ref.file[band].mapset,
	     &cellhead, maptype, nodataval, nodataopt->key, flag_c->answer,
	     default_nodataval) < 0)
	    G_warning(_("Unable to export raster map <%s>"),
		      ref.file[band].name);
    }

    /* Finaly create user required raster format from memory raster if in-memory driver was used */
    if (hMEMDS) {
	hDstDS =
	    GDALCreateCopy(hDriver, output->answer, hMEMDS, FALSE,
			   papszOptions, NULL, NULL);
	if (hDstDS == NULL)
	    G_fatal_error(_("Unable to create raster map <%s> using driver <%s>"),
			  output->answer, format->answer);
    }

    GDALClose(hDstDS);

    if (hMEMDS)
	GDALClose(hMEMDS);

    CSLDestroy(papszOptions);

    G_done_msg(" ");
    exit(EXIT_SUCCESS);
}


int range_check(double min, double max, GDALDataType datatype, char *name)
{
    /* what accuracy to use to print min max for FLOAT32 and FLOAT64? %g enough? */

    switch (datatype) {
    case GDT_Byte:
	if (min < TYPE_BYTE_MIN || max > TYPE_BYTE_MAX) {
	    G_warning(_("Selected GDAL datatype does not cover data range."));
	    G_warning(_("GDAL datatype: %s, range: %d - %d"),
		      GDALGetDataTypeName(datatype), TYPE_BYTE_MIN,
		      TYPE_BYTE_MAX);
	    G_warning(_("Raster map <%s> range: %g - %g"), name, min, max);
	    return 1;
	}
	else
	    return 0;

    case GDT_UInt16:
	if (min < TYPE_UINT16_MIN || max > TYPE_UINT16_MAX) {
	    G_warning(_("Selected GDAL datatype does not cover data range."));
	    G_warning(_("GDAL datatype: %s, range: %d - %d"),
		      GDALGetDataTypeName(datatype), TYPE_UINT16_MIN,
		      TYPE_UINT16_MAX);
	    G_warning(_("Raster map <%s> range: %g - %g"), name, min, max);
	    return 1;
	}
	else
	    return 0;

    case GDT_Int16:
    case GDT_CInt16:
	if (min < TYPE_INT16_MIN || max > TYPE_INT16_MAX) {
	    G_warning(_("Selected GDAL datatype does not cover data range."));
	    G_warning(_("GDAL datatype: %s, range: %d - %d"),
		      GDALGetDataTypeName(datatype), TYPE_INT16_MIN,
		      TYPE_INT16_MAX);
	    G_warning(_("Raster map <%s> range: %g - %g"), name, min, max);
	    return 1;
	}
	else
	    return 0;

    case GDT_Int32:
    case GDT_CInt32:
	if (min < TYPE_INT32_MIN || max > TYPE_INT32_MAX) {
	    G_warning(_("Selected GDAL datatype does not cover data range."));
	    G_warning(_("GDAL datatype: %s, range: %d - %d"),
		      GDALGetDataTypeName(datatype), TYPE_INT32_MIN,
		      TYPE_INT32_MAX);
	    G_warning(_("Raster map <%s> range: %g - %g"), name, min, max);
	    return 1;
	}
	else
	    return 0;

    case GDT_UInt32:
	if (min < TYPE_UINT32_MIN || max > TYPE_UINT32_MAX) {
	    G_warning(_("Selected GDAL datatype does not cover data range."));
	    G_warning(_("GDAL datatype: %s, range: %u - %u"),
		      GDALGetDataTypeName(datatype), TYPE_UINT32_MIN,
		      TYPE_UINT32_MAX);
	    G_warning(_("Raster map <%s> range: %g - %g"), name, min, max);
	    return 1;
	}
	else
	    return 0;

    case GDT_Float32:
    case GDT_CFloat32:
	if (min < TYPE_FLOAT32_MIN || max > TYPE_FLOAT32_MAX) {
	    G_warning(_("Selected GDAL datatype does not cover data range."));
	    G_warning(_("GDAL datatype: %s, range: %g - %g"),
		      GDALGetDataTypeName(datatype), TYPE_FLOAT32_MIN,
		      TYPE_FLOAT32_MAX);
	    G_warning(_("Raster map <%s> range: %g - %g"), name, min, max);
	    return 1;
	}
	else
	    return 0;

    case GDT_Float64:
    case GDT_CFloat64:
	/* not possible because DCELL is FLOAT64, not 128bit floating point, but anyway... */
	if (min < TYPE_FLOAT64_MIN || max > TYPE_FLOAT64_MAX) {
	    G_warning(_("Selected GDAL datatype does not cover data range."));
	    G_warning(_("GDAL datatype: %s, range: %g - %g"),
		      GDALGetDataTypeName(datatype), TYPE_FLOAT64_MIN,
		      TYPE_FLOAT64_MAX);
	    G_warning(_("Raster map <%s> range: %g - %g"), name, min, max);
	    return 1;
	}
	else
	    return 0;

    default:
	return 0;
    }
}

int nullvalue_check(double nodataval, GDALDataType datatype)
{
    /* what accuracy to use to print nodataval for FLOAT32 and FLOAT64? %g enough? */

    switch (datatype) {
    case GDT_Byte:
	if (nodataval < TYPE_BYTE_MIN || nodataval > TYPE_BYTE_MAX) {
	    G_warning(_("Specified nodata value %d is not covered by range of selected GDAL datatype."),
		      (int) nodataval);
	    G_warning(_("GDAL datatype: %s, range: %d - %d"),
		      GDALGetDataTypeName(datatype), TYPE_BYTE_MIN,
		      TYPE_BYTE_MAX);
	    return 1;
	}
	else
	    return 0;

    case GDT_UInt16:
	if (nodataval < TYPE_UINT16_MIN || nodataval > TYPE_UINT16_MAX) {
	    G_warning(_("Specified nodata value %d is not covered by range of selected GDAL datatype."),
		      (int) nodataval);
	    G_warning(_("GDAL datatype: %s, range: %d - %d"),
		      GDALGetDataTypeName(datatype), TYPE_UINT16_MIN,
		      TYPE_UINT16_MAX);
	    return 1;
	}
	else
	    return 0;

    case GDT_Int16:
    case GDT_CInt16:
	if (nodataval < TYPE_INT16_MIN || nodataval > TYPE_INT16_MAX) {
	    G_warning(_("Specified nodata value %d is not covered by range of selected GDAL datatype."),
		      (int) nodataval);
	    G_warning(_("GDAL datatype: %s, range: %d - %d"),
		      GDALGetDataTypeName(datatype), TYPE_INT16_MIN,
		      TYPE_INT16_MAX);
	    return 1;
	}
	else
	    return 0;

    case GDT_Int32:
    case GDT_CInt32:
	if (nodataval < TYPE_INT32_MIN || nodataval > TYPE_INT32_MAX) {
	    G_warning(_("Specified nodata value %lld is not covered by range of selected GDAL datatype."),
		      (long long) nodataval);
	    G_warning(_("GDAL datatype: %s, range: %d - %d"),
		      GDALGetDataTypeName(datatype), TYPE_INT32_MIN,
		      TYPE_INT32_MAX);
	    return 1;
	}
	else
	    return 0;

    case GDT_UInt32:
	if (nodataval < TYPE_UINT32_MIN || nodataval > TYPE_UINT32_MAX) {
	    G_warning(_("Specified nodata value %lld is not covered by range of selected GDAL datatype."),
		      (long long) nodataval);
	    G_warning(_("GDAL datatype: %s, range: %u - %u"),
		      GDALGetDataTypeName(datatype), TYPE_UINT32_MIN,
		      TYPE_UINT32_MAX);
	    return 1;
	}
	else
	    return 0;

    case GDT_Float32:
    case GDT_CFloat32:
	if (nodataval < TYPE_FLOAT32_MIN || nodataval > TYPE_FLOAT32_MAX) {
	    G_warning(_("Specified nodata value %g is not covered by range of selected GDAL datatype."),
		      nodataval);
	    G_warning(_("GDAL datatype: %s, range: %g - %g"),
		      GDALGetDataTypeName(datatype), TYPE_FLOAT32_MIN,
		      TYPE_FLOAT32_MAX);
	    return 1;
	}
	else
	    return 0;

    case GDT_Float64:
    case GDT_CFloat64:
	/* not possible because double is FLOAT64, not 128bit floating point */
	if (nodataval < TYPE_FLOAT64_MIN || nodataval > TYPE_FLOAT64_MAX) {
	    G_warning(_("Specified nodata value %g is not covered by range of selected GDAL datatype."),
		      nodataval);
	    G_warning(_("GDAL datatype: %s, range: %g - %g"),
		      GDALGetDataTypeName(datatype), TYPE_FLOAT64_MIN,
		      TYPE_FLOAT64_MAX);
	    return 1;
	}
	else
	    return 0;

    default:
	return 0;
    }
}
