/*
 *   r.in.bin
 *
 *   Copyright (C) 2000 by the GRASS Development Team
 *   Author: Bob Covill <bcovill tekmap.ns.ca>
 *
 *   This program is free software under the GPL (>=v2)
 *   Read the file COPYING coming with GRASS for details.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <grass/gis.h>
#include "gmt_grd.h"
#include <grass/glocale.h>

typedef unsigned short uint16;
typedef unsigned int uint32;

static double nul_val;

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

int main(int argc, char *argv[])
{
    const char *input;
    const char *output;
    const char *title;
    FILE *fd;
    int cf;
    struct Cell_head cellhd;
    struct History history;
    CELL *cell;
    FCELL *fcell;
    DCELL *dcell;
    RASTER_MAP_TYPE map_type;
    int row, col;
    int nrows = 0, ncols = 0;
    int grass_nrows = 0, grass_ncols = 0;
    int bytes, sflag, swap;
    int no_coord = 0, no_dim = 0;
    void *x_v;
    char *x_c;
    uint16 *x_s;
    uint32 *x_i;
    float *x_f;
    double *x_d;
    struct stat fileinfo;
    int FILE_SIZE;
    const char *err;
    char dummy[2];
    struct GRD_HEADER header;
    struct
    {
	struct Option *input, *output, *title, *bytes,
	    *north, *south, *east, *west, *rows, *cols, *anull;
    } parm;
    struct
    {
	struct Flag *s, *f, *d, *b, *gmt_hd;
    } flag;
    struct GModule *module;
    union
    {
	int testWord;
	char testByte[4];
    } endianTest;
    int swapFlag;

    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    module->keywords = _("raster, import");
    module->description =
	_("Import a binary raster file into a GRASS raster map layer.");


    flag.f = G_define_flag();
    flag.f->key = 'f';
    flag.f->description =
	_("Import as Floating Point Data (default: Integer)");

    flag.d = G_define_flag();
    flag.d->key = 'd';
    flag.d->description =
	_("Import as Double Precision Data (default: Integer)");

    flag.s = G_define_flag();
    flag.s->key = 's';
    flag.s->description = _("Signed data (high bit means negative value)");
    flag.s->guisection = _("Settings");

    flag.b = G_define_flag();
    flag.b->key = 'b';
    flag.b->description = _("Byte Swap the Data During Import");
    flag.b->guisection = _("Settings");

    flag.gmt_hd = G_define_flag();
    flag.gmt_hd->key = 'h';
    flag.gmt_hd->description = _("Get region info from GMT style header");
    flag.gmt_hd->guisection = _("Bounds");

    parm.input = G_define_option();
    parm.input->key = "input";
    parm.input->type = TYPE_STRING;
    parm.input->required = YES;
    parm.input->description = _("Binary raster file to be imported");
    parm.input->gisprompt = "old_file,file,input";

    parm.output = G_define_standard_option(G_OPT_R_OUTPUT);

    parm.title = G_define_option();
    parm.title->key = "title";
    parm.title->key_desc = "phrase";
    parm.title->type = TYPE_STRING;
    parm.title->required = NO;
    parm.title->description = _("Title for resultant raster map");

    parm.bytes = G_define_option();
    parm.bytes->key = "bytes";
    parm.bytes->type = TYPE_INTEGER;
    parm.bytes->answer = "1";
    parm.bytes->required = NO;
    parm.bytes->description = _("Number of bytes per cell (1, 2, 4)");
    parm.bytes->guisection = _("Settings");

    parm.north = G_define_option();
    parm.north->key = "north";
    parm.north->type = TYPE_DOUBLE;
    parm.north->required = NO;
    parm.north->description =
	_("Northern limit of geographic region (outer edge)");
    parm.north->guisection = _("Bounds");

    parm.south = G_define_option();
    parm.south->key = "south";
    parm.south->type = TYPE_DOUBLE;
    parm.south->required = NO;
    parm.south->description =
	_("Southern limit of geographic region (outer edge)");
    parm.south->guisection = _("Bounds");

    parm.east = G_define_option();
    parm.east->key = "east";
    parm.east->type = TYPE_DOUBLE;
    parm.east->required = NO;
    parm.east->description =
	_("Eastern limit of geographic region (outer edge)");
    parm.east->guisection = _("Bounds");

    parm.west = G_define_option();
    parm.west->key = "west";
    parm.west->type = TYPE_DOUBLE;
    parm.west->required = NO;
    parm.west->description =
	_("Western limit of geographic region (outer edge)");
    parm.west->guisection = _("Bounds");

    parm.rows = G_define_option();
    parm.rows->key = "rows";
    parm.rows->type = TYPE_INTEGER;
    parm.rows->required = NO;
    parm.rows->description = _("Number of rows");
    parm.rows->guisection = _("Bounds");

    parm.cols = G_define_option();
    parm.cols->key = "cols";
    parm.cols->type = TYPE_INTEGER;
    parm.cols->required = NO;
    parm.cols->description = _("Number of columns");
    parm.cols->guisection = _("Bounds");

    parm.anull = G_define_option();
    parm.anull->key = "anull";
    parm.anull->type = TYPE_DOUBLE;
    parm.anull->required = NO;
    parm.anull->description = _("Set Value to NULL");
    parm.anull->guisection = _("Settings");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    input = parm.input->answer;
    output = parm.output->answer;
    title = parm.title->answer;

    if (flag.f->answer)
	bytes = 4;
    else if (flag.d->answer)
	bytes = 8;
    else if (sscanf(parm.bytes->answer, "%d%1s", &bytes, dummy) != 1)
	G_fatal_error(_("Parsing bytes per cell"));
    if (bytes <= 0)
	G_fatal_error(_("Bad number of bytes per cell"));
    sflag = flag.s->answer;

    swap = 0;
    if (flag.b->answer) {
	swap = 1;
	G_message(_("Byte Swapping Turned On."));
    }

    /* Check Endian State of Host Computer */
    endianTest.testWord = 1;
    if (endianTest.testByte[0] == 1) {
	swapFlag = 1;		/*true: little endian */
	if (swap == 1)
	    swapFlag = 0;	/* Swapping enabled */
    }
    else {
	swapFlag = 0;
	if (swap == 1)
	    swapFlag = 1;	/* Swapping enabled */
    }

    cellhd.zone = G_zone();
    cellhd.proj = G_projection();

    if (!flag.gmt_hd->answer) {	/* NO GMT header */

	/* Check for optional parameters */
	if (!parm.north->answer && !parm.south->answer && !parm.east->answer
	    && !parm.west->answer) {
	    no_coord = 1;
	    /* No coordinates provided */
	}
	if (!parm.rows->answer && !parm.cols->answer) {
	    no_dim = 1;
	    /* No dimensions provided */
	}
	/* Done check */


	/* CASE 0 - Not enough parameters - exit */
	if (no_dim == 1 && no_coord == 1)
	    G_fatal_error(_("Missing parameters ...\nMust provide at least"
			    "[north= south= east= west=] OR [r=	c=]"));

	/* CASE 1 - All parameters supplied */
	if (no_coord == 0 && no_dim == 0) {	/* Get all parmameters */
	    if (!parm.north->answer || !parm.south->answer ||
		!parm.west->answer || !parm.east->answer)
		G_fatal_error(_("You have to provide all limits of geographic region (n,s,e,w)"));
	    if (!G_scan_northing(parm.north->answer, &cellhd.north, cellhd.proj))
		G_fatal_error(_("Illegal north coordinate <%s>"), parm.north->answer);
	    if (!G_scan_northing(parm.south->answer, &cellhd.south, cellhd.proj))
		G_fatal_error(_("Illegal south coordinate <%s>"), parm.south->answer);
	    if (!G_scan_easting(parm.east->answer, &cellhd.east, cellhd.proj))
		G_fatal_error(_("Illegal east coordinate <%s>"), parm.east->answer);
	    if (!G_scan_easting(parm.west->answer, &cellhd.west, cellhd.proj))
		G_fatal_error(_("Illegal west coordinate <%s>"), parm.west->answer);
	    if (sscanf(parm.rows->answer, "%d%1s", &cellhd.rows, dummy) != 1)
		G_fatal_error(_("Illegal number of rows <%s>"), parm.rows->answer);
	    if (sscanf(parm.cols->answer, "%d%1s", &cellhd.cols, dummy) != 1)
		G_fatal_error(_("Illegal number of columns <%s>"), parm.cols->answer);
	}

	/* CASE 2 - Get only rows and columns and calculate N S E W */
	/* Only works with resolution = 1 */
	if (no_dim == 0 && no_coord == 1) {	/* Get rows and cols only */
	    if (sscanf(parm.rows->answer, "%d%1s", &cellhd.rows, dummy) != 1
		|| cellhd.rows <= 0)
		G_fatal_error(_("Illegal number of rows <%s>"), parm.rows->answer);
	    if (sscanf(parm.cols->answer, "%d%1s", &cellhd.cols, dummy) != 1
		|| cellhd.cols <= 0)
		G_fatal_error(_("Illegal number of columns <%s>"), parm.cols->answer);
	    cellhd.north = (double)cellhd.rows;
	    cellhd.south = 0.;
	    cellhd.east = (double)cellhd.cols;
	    cellhd.west = 0.;
	    G_message(_("Using N=%f S=%f E=%f W=%f"), cellhd.north,
		      cellhd.south, cellhd.east, cellhd.west);
	}


	/* CASE 3 - Get only N S E & W */
	/* No rows and columns supplied - calculate from parameters */
	/* Assumes 1x1 resolution */
	if (no_coord == 0 && no_dim == 1) {	/* Get n, s, e, w */
	    if (!parm.north->answer || !parm.south->answer ||
		!parm.west->answer || !parm.east->answer)
		G_fatal_error(
		    _("You have to provide all limits of geographic region (n,s,e,w)"));
	    if (!G_scan_northing(parm.north->answer, &cellhd.north, cellhd.proj))
		G_fatal_error(_("Illegal north coordinate <%s>"), parm.north->answer);
	    if (!G_scan_northing(parm.south->answer, &cellhd.south, cellhd.proj))
		G_fatal_error(_("Illegal south coordinate <%s>"), parm.south->answer);
	    if (!G_scan_easting(parm.east->answer, &cellhd.east, cellhd.proj))
		G_fatal_error(_("Illegal east coordinate <%s>"), parm.east->answer);
	    if (!G_scan_easting(parm.west->answer, &cellhd.west, cellhd.proj))
		G_fatal_error(_("Illegal west coordinate <%s>"), parm.west->answer);

	    /* Calculate rows and cols */
	    cellhd.rows = (int)cellhd.north - cellhd.south;
	    cellhd.cols = (int)cellhd.east - cellhd.west;
	    G_message(_("Using rows=%d cols=%d\n"), cellhd.rows, cellhd.cols);
	}			/* DONE */
    }


    if (parm.anull->answer != NULL)
	sscanf(parm.anull->answer, "%lf", &nul_val);

    fd = fopen(input, "r");
    if (fd == NULL) {
	perror(input);
	G_usage();
	exit(EXIT_FAILURE);
    }

    /* Read binary GMT style header */
    if (flag.gmt_hd->answer) {
	fread(&header.nx, sizeof(int), 1, fd);
	fread(&header.ny, sizeof(int), 1, fd);
	fread(&header.node_offset, sizeof(int), 1, fd);
	if (swapFlag == 0)
	    fread(&header.node_offset, sizeof(int), 1, fd);

	fread(&header.x_min, sizeof(double), 1, fd);
	fread(&header.x_max, sizeof(double), 1, fd);
	fread(&header.y_min, sizeof(double), 1, fd);
	fread(&header.y_max, sizeof(double), 1, fd);
	fread(&header.z_min, sizeof(double), 1, fd);
	fread(&header.z_max, sizeof(double), 1, fd);
	fread(&header.x_inc, sizeof(double), 1, fd);
	fread(&header.y_inc, sizeof(double), 1, fd);
	fread(&header.z_scale_factor, sizeof(double), 1, fd);
	fread(&header.z_add_offset, sizeof(double), 1, fd);

	fread(&header.x_units, sizeof(char[GRD_UNIT_LEN]), 1, fd);
	fread(&header.y_units, sizeof(char[GRD_UNIT_LEN]), 1, fd);
	fread(&header.z_units, sizeof(char[GRD_UNIT_LEN]), 1, fd);
	fread(&header.title, sizeof(char[GRD_TITLE_LEN]), 1, fd);
	fread(&header.command, sizeof(char[GRD_COMMAND_LEN]), 1, fd);
	fread(&header.remark, sizeof(char[GRD_REMARK_LEN]), 1, fd);

	cellhd.cols = header.nx;
	cellhd.rows = header.ny;
	cellhd.west = header.x_min;
	cellhd.east = header.x_max;
	cellhd.south = header.y_min;
	cellhd.north = header.y_max;
	cellhd.ew_res = header.x_inc;
	cellhd.ns_res = header.y_inc;

	if (swap == 1) {
	    /* Swapping Header Values */
	    SwabLong((uint32 *) & cellhd.cols);
	    SwabLong((uint32 *) & cellhd.rows);
	    SwabDouble(&cellhd.west);
	    SwabDouble(&cellhd.east);
	    SwabDouble(&cellhd.south);
	    SwabDouble(&cellhd.north);
	    SwabDouble(&cellhd.ew_res);
	    SwabDouble(&cellhd.ns_res);
	}

	/* Adjust Cell Header to match resolution */
	if (err = G_adjust_Cell_head(&cellhd, 0, 0)) {
	    G_fatal_error("%s", err);
	    exit(EXIT_FAILURE);
	}
	nrows = header.ny;
	ncols = header.nx;
	grass_nrows = cellhd.rows;
	grass_ncols = cellhd.cols;
    }

    if (!flag.gmt_hd->answer) {
	/* Adjust Cell Header to New Values */
	if (err = G_adjust_Cell_head(&cellhd, 1, 1)) {
	    G_fatal_error("%s", err);
	    exit(EXIT_FAILURE);
	}
	grass_nrows = nrows = cellhd.rows;
	grass_ncols = ncols = cellhd.cols;
    }

    if (G_set_window(&cellhd) < 0)
	exit(3);

    if (grass_nrows != G_window_rows())
	G_fatal_error("rows changed from %d to %d\n", grass_nrows,
		      G_window_rows());

    if (grass_ncols != G_window_cols())
	G_fatal_error("cols changed from %d to %d\n", grass_ncols,
		      G_window_cols());


    /* Find File Size in Byte and Check against byte size */
    if (stat(input, &fileinfo) < 0) {
	perror("fstat");
	exit(1);
    }
    FILE_SIZE = fileinfo.st_size;

    if (flag.gmt_hd->answer) {
	if (swapFlag == 0) {
	    if (FILE_SIZE != (896 + (ncols * nrows * bytes))) {
		G_warning(_("Bytes do not match File size"));
		G_warning(_("File Size %d ... Total Bytes %d"), FILE_SIZE,
			  896 + (ncols * nrows * bytes));
		G_warning(_("Try bytes=%d or adjusting input parameters"),
			  (FILE_SIZE - 896) / (ncols * nrows));
		exit(EXIT_FAILURE);
	    }
	}
	else {
	    if (FILE_SIZE != (892 + (ncols * nrows * bytes))) {
		G_warning(_("Bytes do not match File size"));
		G_warning(_("File Size %d ... Total Bytes %d"), FILE_SIZE,
			  892 + (ncols * nrows * bytes));
		G_warning(_("Try bytes=%d or adjusting input parameters"),
			  (FILE_SIZE - 892) / (ncols * nrows));
		exit(EXIT_FAILURE);
	    }
	}
    }
    else {
	/* No Header */
	if (FILE_SIZE != (ncols * nrows * bytes)) {
	    G_warning(_("Bytes do not match File size"));
	    G_warning(_("File Size %d ... Total Bytes %d"), FILE_SIZE,
		      ncols * nrows * bytes);
	    G_warning(_("Try bytes=%d or adjusting input parameters"),
		      FILE_SIZE / (ncols * nrows));
	    exit(EXIT_FAILURE);
	}
    }

    if (flag.f->answer) {
	map_type = FCELL_TYPE;
	fcell = G_allocate_f_raster_buf();
    }
    else if (flag.d->answer) {
	map_type = DCELL_TYPE;
	dcell = G_allocate_d_raster_buf();
    }
    else {
	cell = G_allocate_c_raster_buf();
	map_type = CELL_TYPE;
    }

    cf = G_open_raster_new(output, map_type);
    if (cf < 0) {
	G_fatal_error(_("Unable to create raster map <%s>"), output);
	exit(EXIT_FAILURE);
    }

    x_v = G_malloc(ncols * bytes);
    x_f = (float *)x_v;
    x_d = (double *)x_v;
    x_i = (uint32 *) x_v;
    x_s = (uint16 *) x_v;
    x_c = (char *)x_v;

    if (cellhd.proj == PROJECTION_LL && cellhd.ew_res / cellhd.ns_res > 10.)	/* TODO: find a reasonable value */
	G_warning(
	   _("East-West (ewres: %f) and North-South (nwres: %f) "
	     "resolution differ significantly. "
	     "Did you assign east= and west= correctly?"),
	     cellhd.ew_res, cellhd.ns_res);

    for (row = 0; row < grass_nrows; row++) {
	if (fread(x_v, bytes * ncols, 1, fd) != 1) {
	    G_unopen_cell(cf);
	    G_fatal_error(_("Conversion failed at row %d"), row);
	    exit(EXIT_FAILURE);
	}

	for (col = 0; col < grass_ncols; col++) {
	    if (flag.f->answer) {
		/* Import Float */
		if (swap)
		    SwabFloat(&x_f[col]);
		fcell[col] = (FCELL) x_f[col];
	    }
	    else if (flag.d->answer) {
		/* Import Double */
		if (swap)
		    SwabDouble(&x_d[col]);
		dcell[col] = (DCELL) x_d[col];
	    }
	    else if (bytes == 1) {
		/* Import 1 byte Byte */
		if (sflag)
		    cell[col] = (CELL) (signed char)x_c[col];
		else
		    cell[col] = (CELL) (unsigned char)x_c[col];
	    }
	    else if (bytes == 2) {
		/* Import 2 byte Short */
		if (swap)
		    SwabShort(&x_s[col]);
		if (sflag)
		    cell[col] = (CELL) (signed short)x_s[col];
		else
		    cell[col] = (CELL) (unsigned short)x_s[col];
	    }
	    else {
		/* Import 4 byte Int */
		if (swap)
		    SwabLong(&x_i[col]);
		if (sflag)
		    cell[col] = (CELL) (signed int)x_i[col];
		else
		    cell[col] = (CELL) (unsigned int)x_i[col];
	    }
	    if (parm.anull->answer) {
		if (flag.f->answer) {
		    if (fcell[col] == (float)nul_val)
			G_set_f_null_value(&fcell[col], 1);
		}
		else {
		    if (cell[col] == (int)nul_val)
			G_set_c_null_value(&cell[col], 1);
		}
	    }
	}

	if (flag.f->answer)
	    G_put_f_raster_row(cf, fcell);
	else if (flag.d->answer)
	    G_put_d_raster_row(cf, dcell);
	else
	    G_put_c_raster_row(cf, cell);

	G_percent(row + 1, nrows, 2);
    }

    G_debug(1, "Creating support files for %s", output);
    G_close_cell(cf);

    if (title)
	G_put_cell_title(output, title);

    G_short_history(output, "raster", &history);
    G_command_history(&history);
    G_write_history(output, &history);


    exit(EXIT_SUCCESS);
}
