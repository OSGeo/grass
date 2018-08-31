/* r.in.mat
 *
 * Input a GRASS raster file from a MAT-File (version 4).
 *
 *   Copyright (C) 2004 by the GRASS Development Team
 *   Author: Hamish Bowman, University of Otago, New Zealand
 *
 *   This program is free software under the GPL (>=v2)
 *   Read the COPYING file that comes with GRASS for details.
 *
 *   Code follows r.out.mat, which follows r.out.bin to a certain
 *   extent, which in turn follows r.out.ascii, which is really old.
 *
 *
 *  ARRAY MUST CONTAIN THE FOLLOWING MATRIX :
 *    map_data             with the map data
 *
 *  AND OPTIONALLY :
 *    map_name             name for new map (max 64 chars, normal rules apply)
 *    map_title            contains map title
 *
 *  THESE MUST BE PRESENT UNLESS USING THE "XY" PROJECTION :
 *    map_northern_edge
 *    map_southern_edge    in decimal form (ie not DDD:MM:SS)
 *    map_eastern_edge
 *    map_western_edge  
 *
 *  ALL OTHER MATRICES WILL BE PASSED OVER. (cleanly, I hope)
 *
 *   tip: Save a version 4 MAT-File with the command "save filename.mat map_* -v4"
 */

/* #define DEBUG */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

/* typedef unsigned short uint16;
   typedef unsigned int uint32; */

/* is_nan():  fn to test if incoming data point is either a IEEE NaN or a GRASS CELL null */
int is_nan(void *, RASTER_MAP_TYPE);


int main(int argc, char *argv[])
{

    int i, row, col;		/* counters */

    int machine_endianness, file_endianness, endian_mismatch;	/* 0=little, 1=big */
    int data_format;		/* 0=double  1=float  2=32bit signed int  5=8bit unsigned int (ie text) */
    int data_type;		/* 0=numbers  1=text */
    int format_block;		/* combo of endianness, 0, data_format, and type */
    int realflag = 0;		/* 0=only real values used */

    /* should type be specifically uint32 ??? */

    char array_name[65];	/* 65 = 64 + null-terminator */
    int name_len;
    int mrows, ncols;		/* text/data/map array dimensions */

    int *pval_i;		/* for misc use */
    float *pval_f;		/* for misc use */
    double *pval_d;		/* for misc use */
    char c;			/* for misc use */

    char map_name[65], map_title[1024];	/* 65 = 64 + null-terminator */
    double map_name_d[1024];	/* I'm not sure why you'd save char strings as double, but whatever */

    int have_name, have_data, have_title, have_n, have_s, have_e, have_w;

    char *infile, *outfile;
    struct Cell_head region;
    void *raster, *rastline_ptr, *array_data, *array_ptr;
    RASTER_MAP_TYPE map_type;
    struct History history;

    struct Option *inputfile, *outputfile;
    struct GModule *module;

    int cf;
    FILE *fp1;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("import"));
    module->description =
	_("Imports a binary MAT-File(v4) to a GRASS raster.");


    /* Define the different options */
    inputfile = G_define_standard_option(G_OPT_F_INPUT);
    inputfile->required = YES;
    inputfile->gisprompt = "old,mat,file";
    inputfile->description = _("Name of input MAT-File(v4)");

    outputfile = G_define_standard_option(G_OPT_R_OUTPUT);
    outputfile->required = NO;
    outputfile->description = _("Name for output raster map (override)");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

  /******  SETUP  ****************************************************/
    /* Check Endian State of Host Computer */
    if (G_is_little_endian())
	machine_endianness = 0;	/* ie little endian */
    else
	machine_endianness = 1;	/* ie big endian */
    G_debug(1, "Machine is %s endian.",
	    machine_endianness ? "big" : "little");

    infile = inputfile->answer;
    outfile = outputfile->answer;

    /* open bin file for reading */
    fp1 = fopen(infile, "rb");
    if (NULL == fp1)
	G_fatal_error(_("Unable to open input file <%s>"), infile);

    have_name = have_data = have_title = 0;
    have_n = have_s = have_e = have_w = 0;

    /* Check Endian State of File */
    fread(&format_block, sizeof(int), 1, fp1);
    G_fseek(fp1, 0, SEEK_SET);	/* frewind() */

    file_endianness = format_block / 1000;	/* 0=little, 1=big */
    if (file_endianness != machine_endianness)
	endian_mismatch = 1;
    else
	endian_mismatch = 0;

    G_debug(1, "File is %s endian.\n", file_endianness ? "big" : "little");

    if (format_block > 51)
	G_warning
	    ("Only little endian MAT-File(v4) binaries have been tested so far! Probably won't work.");



  /******  READ MAP  ****************************************************/
    G_verbose_message(_("Reading MAT-File..."));

    while (!feof(fp1)) {

	/* scan for needed array variables */
	fread(&format_block, sizeof(int), 1, fp1);

	if (feof(fp1))
	    break;

	/* 4 byte data format block = endianness*1000 + data_format*10 + data_type */
	/*   0=double   1=float   2=32bit signed int   5=8bit unsigned int(text)   */
	data_format = format_block / 10;
	if (data_format != 0 && data_format != 1 &&
	    data_format != 2 && data_format != 5)
	    G_fatal_error("format [%d]", data_format);

	data_type = format_block - data_format * 10;	/* 0=numbers  1=text */
	if (data_type != 0 && data_type != 1)
	    G_fatal_error("type [%d]", data_type);


	/* 4 byte number of rows & columns */
	fread(&mrows, sizeof(int), 1, fp1);
	fread(&ncols, sizeof(int), 1, fp1);
	if (mrows < 1 || ncols < 1)
	    G_fatal_error(_("Array contains no data"));

	/* 4 byte real/imag flag   0=real vals only */
	fread(&realflag, sizeof(int), 1, fp1);
	if (realflag != 0)
	    G_fatal_error(_("Array contains imaginary data"));


	/* length of array_name+1 */
	fread(&name_len, sizeof(int), 1, fp1);
	if (name_len < 1)
	    G_fatal_error(_("Invalid array name"));

	/* array name */
	for (i = 0; i < 64; i++) {
	    fread(&c, sizeof(char), 1, fp1);
	    array_name[i] = c;
	    if (c == '\0')
		break;
	}

	G_debug(3, "array name     = [%s]", array_name);
	G_debug(3, "  format block = [%04d]", format_block);
	G_debug(3, "  data format  = [%d]", data_format);
	G_debug(3, "  data type    = [%d]", data_type);
	G_debug(3, "  rows         = [%d]", mrows);
	G_debug(3, "  cols         = [%d]", ncols);

	if (strcmp(array_name, "map_name") == 0) {
	    have_name = 1;
	    if (mrows != 1 || ncols > 64 || data_type != 1)
		G_fatal_error(_("Invalid 'map_name' array"));

	    if (data_format == 5)
		fread(&map_name, sizeof(char), ncols, fp1);
	    else if (data_format == 0) {	/* sigh.. */
		fread(&map_name_d, sizeof(double), ncols, fp1);
		for (i = 0; i < ncols; i++)
		    map_name[i] = (char)map_name_d[i];
	    }
	    else
		G_fatal_error(_("Error reading 'map_name' array"));

	    map_name[ncols] = '\0';
	    G_strip(map_name);	/* remove leading and trailing whitespace */
	    G_debug(1, "map name= <%s>", map_name);
	}

	else if (strcmp(array_name, "map_northern_edge") == 0) {
	    have_n = 1;
	    if (mrows != 1 || ncols != 1 || data_format != 0 ||
		data_type != 0)
		G_fatal_error(_("Invalid 'map_northern_edge' array"));
	    fread(&region.north, sizeof(double), 1, fp1);
	    G_debug(1, "northern edge=%f", region.north);
	}

	else if (strcmp(array_name, "map_southern_edge") == 0) {
	    have_s = 1;
	    if (mrows != 1 || ncols != 1 || data_format != 0 ||
		data_type != 0)
		G_fatal_error(_("Invalid 'map_southern_edge' array"));
	    fread(&region.south, sizeof(double), 1, fp1);
	    G_debug(1, "southern edge=%f", region.south);
	}

	else if (strcmp(array_name, "map_eastern_edge") == 0) {
	    have_e = 1;
	    if (mrows != 1 || ncols != 1 || data_format != 0 ||
		data_type != 0)
		G_fatal_error(_("Invalid 'map_eastern_edge' array"));
	    fread(&region.east, sizeof(double), 1, fp1);
	    G_debug(1, "eastern edge=%f", region.east);
	}

	else if (strcmp(array_name, "map_western_edge") == 0) {
	    have_w = 1;
	    if (mrows != 1 || ncols != 1 || data_format != 0 ||
		data_type != 0)
		G_fatal_error(_("Invalid 'map_western_edge' array"));
	    fread(&region.west, sizeof(double), 1, fp1);
	    G_debug(1, "western edge=%f", region.west);
	}

	else if (strcmp(array_name, "map_title") == 0) {
	    have_title = 1;
	    if (mrows != 1 || ncols > 1023 || data_type != 1)
		G_fatal_error(_("Invalid 'map_title' array"));

	    if (data_format == 5)
		fread(&map_title, sizeof(char), ncols, fp1);
	    else if (data_format == 0) {	/* sigh.. */
		fread(&map_name_d, sizeof(double), ncols, fp1);	/* note reusing variable */
		for (i = 0; i < ncols; i++)
		    map_title[i] = (char)map_name_d[i];
	    }
	    else
		G_fatal_error(_("Error reading 'map_title' array"));

	    map_title[ncols] = '\0';
	    G_strip(map_title);	/* remove leading and trailing whitespace */
	    G_debug(1, "map title= [%s]", map_title);
	}

	else if (strcmp(array_name, "map_data") == 0) {
	    have_data = 1;
	    region.rows = (int)mrows;
	    region.cols = (int)ncols;

	    if (mrows < 1 || ncols < 1 || data_format > 2 || data_type != 0)
		G_fatal_error(_("Invalid 'map_data' array"));

	    switch (data_format) {
		/*   0=double       1=float   2=32bit signed int   5=8bit unsigned int(text)   */
	    case 0:
		G_debug(1, " double map");
		map_type = DCELL_TYPE;
		array_data =
		    G_calloc(mrows * (ncols + 1), Rast_cell_size(map_type));
		fread(array_data, sizeof(double), mrows * ncols, fp1);
		break;
	    case 1:
		G_debug(1, " float map");
		map_type = FCELL_TYPE;
		array_data =
		    G_calloc(mrows * (ncols + 1), Rast_cell_size(map_type));
		fread(array_data, sizeof(float), mrows * ncols, fp1);
		break;
	    case 2:
		G_debug(1, " int map");
		map_type = CELL_TYPE;
		array_data =
		    G_calloc(mrows * (ncols + 1), Rast_cell_size(map_type));
		fread(array_data, sizeof(int), mrows * ncols, fp1);
		break;
	    default:
		G_fatal_error(_("Please contact the GRASS development team"));
	    }
	}			/* endif map_data */

	else {
	    G_important_message(_("Skipping unknown array '%s'"), array_name);
	    switch (data_format) {
		/*   0=double       1=float   2=32bit signed int   5=8bit unsigned int(text)   */
	    case 0:
		G_fseek(fp1, mrows * ncols * sizeof(double), SEEK_CUR);
		break;
	    case 1:
		G_fseek(fp1, mrows * ncols * sizeof(float), SEEK_CUR);
		break;
	    case 2:
		G_fseek(fp1, mrows * ncols * sizeof(int), SEEK_CUR);
		break;
	    case 5:
		G_fseek(fp1, mrows * ncols * sizeof(char), SEEK_CUR);
		break;
	    default:
		G_fatal_error("unusual array");
	    }
	}

	G_debug(3, "Read array '%s' [%d,%d] format=%d type=%d\n",
		array_name, ncols, mrows, data_format, data_type);

    }				/* while !EOF */


  /******  WRITE MAP  ****************************************************/
    if (0 == have_data)
	G_fatal_error(_("No 'map_data' array found in <%s>"), infile);


    /* set map name */
    if (have_name) {
	if (outfile) {
	    if (0 != strcmp(outfile, map_name))
		G_message(_("Setting map name to <%s> which overrides <%s>"),
			  outfile, map_name);
	    strncpy(map_name, outfile, 61);
	}
    }
    else {
	if (outfile) {
	    G_verbose_message(_("Setting map name to <%s>"), outfile);
	    strncpy(map_name, outfile, 61);
	}
	else {
	    G_message(_("No 'map_name' array found; using <MatFile>"));
	    strcpy(map_name, "MatFile");
	}
    }

    G_strip(map_name);		/* remove leading and trailing whitespace */


    /* set region info */
    if (G_projection() != PROJECTION_XY) {
	if ((0 == have_n) || (0 == have_s) || (0 == have_e) || (0 == have_w))
	    G_fatal_error(_("Missing bound"));
    }
    else {
	if ((0 == have_n) || (0 == have_s) || (0 == have_e) || (0 == have_w)) {
	    G_warning(_("Using default bounds"));
	    region.north = (double)region.rows;
	    region.south = 0.;
	    region.east = (double)region.cols;
	    region.west = 0.;
	}
    }

    region.proj = G_projection();
    region.zone = G_zone();
    G_adjust_Cell_head(&region, 1, 1);
    Rast_set_window(&region);

    G_verbose_message("");
    G_verbose_message(_("Map <%s> bounds set to:"), map_name);
    G_verbose_message(_("northern edge=%f"), region.north);
    G_verbose_message(_("southern edge=%f"), region.south);
    G_verbose_message(_("eastern edge=%f"), region.east);
    G_verbose_message(_("western edge=%f"), region.west);
    G_verbose_message(_("nsres=%f"), region.ns_res);
    G_verbose_message(_("ewres=%f"), region.ew_res);
    G_verbose_message(_("rows=%d"), region.rows);
    G_verbose_message(_("cols=%d"), region.cols);
    G_verbose_message("");

    /* prep memory */
    raster = Rast_allocate_buf(map_type);

    cf = Rast_open_new(map_name, map_type);

    /* write new raster map */
    G_verbose_message(_("Writing new raster map..."));

    mrows = region.rows;
    ncols = region.cols;

    for (row = 0; row < mrows; row++) {
	rastline_ptr = raster;
	for (col = 0; col < ncols; col++) {
	    array_ptr = array_data;
	    array_ptr =
		G_incr_void_ptr(array_ptr,
				(row +
				 col * mrows) * Rast_cell_size(map_type));

	    if (is_nan(array_ptr, map_type))
		Rast_set_null_value(rastline_ptr, 1, map_type);
	    else {
		switch (map_type) {
		case CELL_TYPE:
		    pval_i = (int *)array_ptr;
		    Rast_set_c_value(rastline_ptr, (CELL) pval_i[0],
					 map_type);
		    break;
		case FCELL_TYPE:
		    pval_f = (float *)array_ptr;
		    Rast_set_f_value(rastline_ptr, (FCELL) pval_f[0],
					 map_type);
		    break;
		case DCELL_TYPE:
		    pval_d = (double *)array_ptr;
		    Rast_set_d_value(rastline_ptr, (DCELL) pval_d[0],
					 map_type);
		    break;
		default:
		    Rast_close(cf);
		    G_fatal_error(_("Please contact the GRASS development team"));
		}
	    }
	    rastline_ptr =
		G_incr_void_ptr(rastline_ptr, Rast_cell_size(map_type));
	}

#ifdef DEBUG
	fprintf(stderr, "row[%d]=[", row);
	rastline_ptr = raster;
	for (col = 0; col < ncols; col++) {
	    if (Rast_is_null_value(rastline_ptr, map_type))
		fprintf(stderr, "_");
	    else
		fprintf(stderr, "+");
	    rastline_ptr =
		G_incr_void_ptr(rastline_ptr, Rast_cell_size(map_type));
	}
	fprintf(stderr, "]\n");
#endif

	Rast_put_row(cf, raster, map_type);

	G_percent(row, mrows, 5);
    }

    G_percent(row, mrows, 5);	/* finish it off */

    Rast_close(cf);

    G_free(array_data);
    G_free(raster);


    if (!have_title)
	strncpy(map_title, infile, 1023);

    Rast_put_cell_title(map_name, map_title);

    Rast_short_history(map_name, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(map_name, &history);

    G_done_msg("");

    exit(EXIT_SUCCESS);
}



int is_nan(void *p, RASTER_MAP_TYPE dtype)
{
    /* fn to test if incoming data point is either a IEEE NaN or a GRASS CELL null
     *
     *   Takes a value of type RASTER_MAP_TYPE stored at memory address "p"
     *   Returns 1 if we think it is a null, 0 otherwise
     */

    /* please improve */

    float *pval_f;
    double *pval_d;

    switch (dtype) {
    case CELL_TYPE:		/* int doesn't have a IEEE NaN value, but we'll accept GRASS's version */
	if (Rast_is_null_value(p, dtype))
	    return 1;
	break;
    case FCELL_TYPE:
	pval_f = (float *)p;
	if (pval_f[0] != pval_f[0])
	    return 1;
	break;
    case DCELL_TYPE:
	pval_d = (double *)p;
	if (pval_d[0] != pval_d[0])
	    return 1;
	break;
    default:
	G_fatal_error(_("Please contact the GRASS development team"));
    }

    /* otherwise */
    return 0;
}


#ifdef NOTYET
/* for endian swapping, sometime in the future .. */
static void SwabShort(uint16 * wp)
{
    register char *cp = (char *)wp;
    int t;

    t = cp[1];
    cp[1] = cp[0];
    cp[0] = t;
}

static void SwabLong(uint32 * lp)
{
    register char *cp = (char *)lp;
    int t;

    t = cp[3];
    cp[3] = cp[0];
    cp[0] = t;
    t = cp[2];
    cp[2] = cp[1];
    cp[1] = t;
}

static void SwabFloat(float *fp)
{
    SwabLong((uint32 *) fp);
}

static void SwabDouble(double *dp)
{
    register char *cp = (char *)dp;
    int t;

    t = cp[7];
    cp[7] = cp[0];
    cp[0] = t;
    t = cp[6];
    cp[6] = cp[1];
    cp[1] = t;
    t = cp[5];
    cp[5] = cp[2];
    cp[2] = t;
    t = cp[4];
    cp[4] = cp[3];
    cp[3] = t;
}
#endif
