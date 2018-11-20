
/****************************************************************************
 *
 * MODULE:       r.in.bin
 * AUTHOR(S):    Jacques Bouchard, France (bouchard@onera.fr)
 *               Bob Covill <bcovill tekmap.ns.ca>
 *               Markus Metz
 * PURPOSE:      Import binary files
 * COPYRIGHT:    (C) 2000 - 2014 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/stat.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "gmt_grd.h"

enum fliphv {
    FLIP_H = 1,
    FLIP_V = 2,
};

static void swap_2(void *p)
{
    unsigned char *q = p;
    unsigned char t;
    t = q[0]; q[0] = q[1]; q[1] = t;
}

static void swap_4(void *p)
{
    unsigned char *q = p;
    unsigned char t;
    t = q[0]; q[0] = q[3]; q[3] = t;
    t = q[1]; q[1] = q[2]; q[2] = t;
}

static void swap_8(void *p)
{
    unsigned char *q = p;
    unsigned char t;
    t = q[0]; q[0] = q[7]; q[7] = t;
    t = q[1]; q[1] = q[6]; q[6] = t;
    t = q[2]; q[2] = q[5]; q[5] = t;
    t = q[3]; q[3] = q[4]; q[4] = t;
}

static void read_int(FILE *fp, int swap_flag, int *x)
{
    if (fread(x, 4, 1, fp) != 1)
	G_fatal_error(_("Error reading data"));

    if (swap_flag)
	swap_4(x);
}

static void read_double(FILE *fp, int swap_flag, double *x)
{
    if (fread(x, 8, 1, fp) != 1)
	G_fatal_error(_("Error reading data"));

    if (swap_flag)
	swap_8(x);
}

static void read_gmt_header(struct GRD_HEADER *header, int swap_flag, FILE *fp)
{
    read_int(fp, swap_flag, &header->nx);
    read_int(fp, swap_flag, &header->ny);
    read_int(fp, swap_flag, &header->node_offset);

    read_double(fp, swap_flag, &header->x_min);
    read_double(fp, swap_flag, &header->x_max);
    read_double(fp, swap_flag, &header->y_min);
    read_double(fp, swap_flag, &header->y_max);
    read_double(fp, swap_flag, &header->z_min);
    read_double(fp, swap_flag, &header->z_max);
    read_double(fp, swap_flag, &header->x_inc);
    read_double(fp, swap_flag, &header->y_inc);
    read_double(fp, swap_flag, &header->z_scale_factor);
    read_double(fp, swap_flag, &header->z_add_offset);

    fread(&header->x_units, sizeof(char[GRD_UNIT_LEN]),    1, fp);
    fread(&header->y_units, sizeof(char[GRD_UNIT_LEN]),    1, fp);
    fread(&header->z_units, sizeof(char[GRD_UNIT_LEN]),    1, fp);
    fread(&header->title,   sizeof(char[GRD_TITLE_LEN]),   1, fp);
    fread(&header->command, sizeof(char[GRD_COMMAND_LEN]), 1, fp);
    fread(&header->remark,  sizeof(char[GRD_REMARK_LEN]),  1, fp);
}

static void get_gmt_header(const struct GRD_HEADER *header, struct Cell_head *region)
{
    region->cols   = header->nx;
    region->rows   = header->ny;
    region->west   = header->x_min;
    region->east   = header->x_max;
    region->south  = header->y_min;
    region->north  = header->y_max;
    region->ew_res = header->x_inc;
    region->ns_res = header->y_inc;
}

static void convert_cell(
    DCELL *out_cell, unsigned char *in_cell,
    int is_fp, int is_signed, int bytes, int swap_flag)
{
    if (swap_flag) {
	switch (bytes) {
	case 1:				break;
	case 2:	swap_2(in_cell);	break;
	case 4:	swap_4(in_cell);	break;
	case 8:	swap_8(in_cell);	break;
	}
    }

    if (is_fp) {
	switch (bytes) {
	case 4:
	    *out_cell = (DCELL) *(float *) in_cell;
	    break;
	case 8:
	    *out_cell = (DCELL) *(double *) in_cell;
	    break;
	}
    }
    else if (is_signed) {
	switch (bytes) {
	case 1:
	    *out_cell = (DCELL) *(signed char *) in_cell;
	    break;
	case 2:
	    *out_cell = (DCELL) *(short *) in_cell;
	    break;
	case 4:
	    *out_cell = (DCELL) *(int *) in_cell;
	    break;
#ifdef HAVE_LONG_LONG_INT
	case 8:
	    *out_cell = (DCELL) *(long long *) in_cell;
	    break;
#endif
	}
    }
    else {
	switch (bytes) {
	case 1:
	    *out_cell = (DCELL) *(unsigned char *) in_cell;
	    break;
	case 2:
	    *out_cell = (DCELL) *(unsigned short *) in_cell;
	    break;
	case 4:
	    *out_cell = (DCELL) *(unsigned int *) in_cell;
	    break;
#ifdef HAVE_LONG_LONG_INT
	case 8:
	    *out_cell = (DCELL) *(unsigned long long *) in_cell;
	    break;
#endif
	}
    }
}

static void convert_row(
    DCELL *raster, unsigned char *in_buf, int ncols,
    int is_fp, int is_signed, int bytes, int swap_flag,
    double null_val, int flip)
{
    unsigned char *ptr = in_buf;
    int i, i2;

    for (i = 0; i < ncols; i++) {
	DCELL x;

	convert_cell(&x, ptr, is_fp, is_signed, bytes, swap_flag);
	i2 = i;
	if (flip & FLIP_H)
	    i2 = ncols - i - 1;
	if (x == null_val)
	    Rast_set_d_null_value(&raster[i2], 1);
	else
	    raster[i2] = x;
	ptr += bytes;
    }
}

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct
    {
	struct Option *input;
	struct Option *output;
	struct Option *null;
	struct Option *bytes;
	struct Option *hbytes;
	struct Option *bands;
	struct Option *order;
	struct Option *title;
	struct Option *north;
	struct Option *south;
	struct Option *east;
	struct Option *west;
	struct Option *rows;
	struct Option *cols;
	struct Option *flip;
    } parm;
    struct
    {
	struct Flag *float_in;
	struct Flag *double_in;
	struct Flag *gmt_hd;
	struct Flag *sign;
	struct Flag *swap;
    } flag;
    char *desc = NULL;
    const char *input;
    const char *outpre;
    char output[GNAME_MAX];
    const char *title;
    double null_val = 0.0/0.0;
    int is_fp;
    int is_signed;
    int bytes, hbytes;
    int band, nbands, bsize;
    int order;
    int swap_flag;
    int i, flip;
    struct Cell_head cellhd;
    int nrows, ncols;
    int grass_nrows, grass_ncols;
    unsigned char *in_buf;
    DCELL *out_buf;
    RASTER_MAP_TYPE map_type;
    int fd;
    FILE *fp;
    off_t file_size, band_off;
    struct GRD_HEADER header;
    int row;
    struct History history;
    off_t expected;

    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("import"));
    module->description =
	_("Import a binary raster file into a GRASS raster map layer.");

    flag.float_in = G_define_flag();
    flag.float_in->key = 'f';
    flag.float_in->description =
	_("Import as floating-point data (default: integer)");

    flag.double_in = G_define_flag();
    flag.double_in->key = 'd';
    flag.double_in->description =
	_("Import as double-precision floating-point data (default: integer)");

    flag.sign = G_define_flag();
    flag.sign->key = 's';
    flag.sign->description = _("Signed data (two's complement)");
    flag.sign->guisection = _("Settings");

    flag.swap = G_define_flag();
    flag.swap->key = 'b';
    flag.swap->description = _("Byte swap the data during import");
    flag.swap->guisection = _("Settings");

    flag.gmt_hd = G_define_flag();
    flag.gmt_hd->key = 'h';
    flag.gmt_hd->description = _("Get region info from GMT style header");
    flag.gmt_hd->guisection = _("Bounds");

    parm.input = G_define_standard_option(G_OPT_F_BIN_INPUT);
    parm.input->description = _("Name of binary raster file to be imported");

    parm.output = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.output->description = _("Output name or prefix if several bands are imported");

    parm.title = G_define_option();
    parm.title->key = "title";
    parm.title->key_desc = "phrase";
    parm.title->type = TYPE_STRING;
    parm.title->required = NO;
    parm.title->description = _("Title for resultant raster map");

    parm.bytes = G_define_option();
    parm.bytes->key = "bytes";
    parm.bytes->type = TYPE_INTEGER;
    parm.bytes->required = NO;
    parm.bytes->options = "1,2,4,8";
    parm.bytes->description = _("Number of bytes per cell");
    parm.bytes->guisection = _("Settings");

    parm.hbytes = G_define_option();
    parm.hbytes->key = "header";
    parm.hbytes->type = TYPE_INTEGER;
    parm.hbytes->required = NO;
    parm.hbytes->answer = "0";
    parm.hbytes->description = _("Header size in bytes");
    parm.hbytes->guisection = _("Settings");

    parm.bands = G_define_option();
    parm.bands->key = "bands";
    parm.bands->type = TYPE_INTEGER;
    parm.bands->required = NO;
    parm.bands->answer = "1";
    parm.bands->label = _("Number of bands in input file");
    parm.bands->description = _("Bands must be in band-sequential order");
    parm.bands->guisection = _("Settings");

    parm.order = G_define_option();
    parm.order->key = "order";
    parm.order->type = TYPE_STRING;
    parm.order->required = NO;
    parm.order->options = "big,little,native,swap";
    parm.order->description = _("Output byte order");
    parm.order->answer = "native";

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

    parm.null = G_define_option();
    parm.null->key = "anull";
    parm.null->type = TYPE_DOUBLE;
    parm.null->required = NO;
    parm.null->description = _("Set Value to NULL");
    parm.null->guisection = _("Settings");

    parm.flip = G_define_option();
    parm.flip->key = "flip";
    parm.flip->type = TYPE_STRING;
    parm.flip->required = NO;
    parm.flip->options = "h,v";
    parm.flip->multiple = YES;
    parm.flip->label = _("Flip input horizontal and/or vertical");
    G_asprintf(&desc,
	       "h;%s;v;%s",
	       _("Flip input horizontal (East - West)"),
	       _("Flip input vertical (North - South)"));
    parm.flip->descriptions = desc;
    parm.flip->guisection = _("Settings");


    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    input = parm.input->answer;
    outpre = parm.output->answer;
    title = parm.title->answer;

    nbands = atoi(parm.bands->answer);
    if (nbands < 1)
	G_fatal_error(_("Option %s must be > 0"), parm.bands->key);
    hbytes = atoi(parm.hbytes->answer);
    if (hbytes < 0)
	G_fatal_error(_("Option %s must be >= 0"), parm.hbytes->key);

    if (G_strcasecmp(parm.order->answer, "big") == 0)
	order = 0;
    else if (G_strcasecmp(parm.order->answer, "little") == 0)
	order = 1;
    else if (G_strcasecmp(parm.order->answer, "native") == 0)
	order = G_is_little_endian() ? 1 : 0;
    else if (G_strcasecmp(parm.order->answer, "swap") == 0)
	order = G_is_little_endian() ? 0 : 1;

    if (flag.swap->answer) {
	if (strcmp(parm.order->answer, "native") != 0)
	    G_fatal_error(_("-%c and %s= are mutually exclusive"),
			    flag.swap->key, parm.order->key);
	order = G_is_little_endian() ? 0 : 1;
    }

    if (flag.gmt_hd->answer && parm.flip->answer)
	G_fatal_error(_("-%c and %s= are mutually exclusive"),
		      flag.gmt_hd->key, parm.flip->key);
    if (flag.gmt_hd->answer && hbytes > 0)
	G_warning(_("Option %s= is ignored if -%c is set"),
		    parm.hbytes->key, flag.gmt_hd->key);
    if (flag.gmt_hd->answer && nbands > 1)
	G_warning(_("Option %s= is ignored if -%c is set"),
		    parm.bands->key, flag.gmt_hd->key);

    swap_flag = order == (G_is_little_endian() ? 0 : 1);

    is_signed = !!flag.sign->answer;

    flip = 0;
    if (parm.flip->answers) {
	for (i = 0; parm.flip->answers[i]; i++) {
	    if (parm.flip->answers[i][0] == 'h')
		flip |= FLIP_H;
	    if (parm.flip->answers[i][0] == 'v')
		flip |= FLIP_V;
	}
    }

    is_fp = 0;
    bytes = 0;

    if (parm.bytes->answer)
	bytes = atoi(parm.bytes->answer);

    if (flag.float_in->answer && flag.double_in->answer)
	G_fatal_error(_("-%c and -%c are mutually exclusive"),
			flag.float_in->key, flag.double_in->key);

    if (flag.float_in->answer) {
	if (bytes && bytes < 4)
	    G_fatal_error(_("-%c incompatible with %s=%d; must be 4 or 8"),
			    flag.float_in->key, parm.bytes->key, bytes);
	if (!bytes)
	    bytes = 4;
	is_fp = 1;
    }

    if (flag.double_in->answer) {
	if (bytes && bytes != 8)
	    G_fatal_error(_("-%c incompatible with %s=%d; must be 8"),
			    flag.double_in->key, parm.bytes->key, bytes);
	if (!bytes)
	    bytes = 8;
	is_fp = 1;
    }

    if (!is_fp && !bytes)
	G_fatal_error(_("%s= required for integer data"), parm.bytes->key);

#ifndef HAVE_LONG_LONG_INT
    if (!is_fp && bytes > 4)
	G_fatal_error(_("Integer input doesn't support %s=8 in this build"),
			parm.bytes->key);
#endif

    if (bytes != 1 && bytes != 2 && bytes != 4 && bytes != 8)
	G_fatal_error(_("%s= must be 1, 2, 4 or 8"), parm.bytes->key);

    if (parm.null->answer && G_strcasecmp(parm.null->answer, "nan") != 0)
	null_val = atof(parm.null->answer);

    cellhd.zone = G_zone();
    cellhd.proj = G_projection();

    if (!flag.gmt_hd->answer) {
	/* NO GMT header */
	int num_bounds;

	if (!parm.rows->answer || !parm.cols->answer)
	    G_fatal_error(_("Either -%c or %s= and %s= must be given"),
			    flag.gmt_hd->key, parm.rows->key, parm.cols->key);

	num_bounds = !!parm.north->answer + !!parm.south->answer +
	    !!parm.east->answer + !!parm.west->answer;
	if (num_bounds != 0 && num_bounds != 4)
	    G_fatal_error(_("Either all or none of %s=, %s=, %s= and %s= must be given"),
			    parm.north->key, parm.south->key,
			    parm.east->key, parm.west->key);

	cellhd.rows = atoi(parm.rows->answer);
	cellhd.cols = atoi(parm.cols->answer);

	if (num_bounds > 0) {
	    if (!G_scan_northing(parm.north->answer, &cellhd.north, cellhd.proj))
		G_fatal_error(_("Illegal north coordinate <%s>"), parm.north->answer);
	    if (!G_scan_northing(parm.south->answer, &cellhd.south, cellhd.proj))
		G_fatal_error(_("Illegal south coordinate <%s>"), parm.south->answer);
	    if (!G_scan_easting(parm.east->answer, &cellhd.east, cellhd.proj))
		G_fatal_error(_("Illegal east coordinate <%s>"), parm.east->answer);
	    if (!G_scan_easting(parm.west->answer, &cellhd.west, cellhd.proj))
		G_fatal_error(_("Illegal west coordinate <%s>"), parm.west->answer);
	}
    }

    fp = fopen(input, "rb");
    if (!fp)
	G_fatal_error(_("Unable to open <%s>"), input);

    /* Find File Size in Byte and Check against byte size */
    G_fseek(fp, 0, SEEK_END);
    file_size = G_ftell(fp);
    G_fseek(fp, 0, SEEK_SET);

    /* Read binary GMT style header */
    if (flag.gmt_hd->answer) {
	read_gmt_header(&header, swap_flag, fp);
	get_gmt_header(&header, &cellhd);
	hbytes = 892;
	nbands = 1;
    }

    /* Adjust Cell Header to New Values */
    G_adjust_Cell_head(&cellhd, 1, 1);

    if (cellhd.proj == PROJECTION_LL && cellhd.ew_res / cellhd.ns_res > 10.)
	/* TODO: find a reasonable value */
	G_warning(
	    _("East-West (ewres: %f) and North-South (nsres: %f) "
	      "resolution differ significantly. "
	      "Did you assign %s= and %s= correctly?"),
	    cellhd.ew_res, cellhd.ns_res, parm.east->key, parm.west->key);

    grass_nrows = nrows = cellhd.rows;
    grass_ncols = ncols = cellhd.cols;

    Rast_set_window(&cellhd);

    if (grass_nrows != Rast_window_rows())
	G_fatal_error("rows changed from %d to %d",
		      grass_nrows, Rast_window_rows());

    if (grass_ncols != Rast_window_cols())
	G_fatal_error("cols changed from %d to %d",
		      grass_ncols, Rast_window_cols());

    expected = (off_t) ncols * nrows * bytes * nbands + hbytes;

    if (file_size != expected) {
	G_warning(_("File Size %"PRI_OFF_T" ... Total Bytes %"PRI_OFF_T),
		  file_size, expected);
	G_fatal_error(_("Bytes do not match file size"));
    }

    in_buf = G_malloc(ncols * bytes);
    out_buf = Rast_allocate_d_buf();

    map_type = is_fp ? (bytes > 4 ? DCELL_TYPE : FCELL_TYPE) : CELL_TYPE;

    in_buf = G_malloc(ncols * bytes);
    out_buf = Rast_allocate_d_buf();

    bsize = log10(nbands) + 1;
    if (!flag.gmt_hd->answer && hbytes > 0)
	G_fseek(fp, hbytes, SEEK_SET);

    for (band = 1; band <= nbands; band++) {
	
	if (nbands > 1) {
	    G_message(_("Importing band %d..."), band);
	    sprintf(output, "%s%0*d", outpre, bsize, band);
	}
	else
	    sprintf(output, "%s", outpre);

	fd = Rast_open_new(output, map_type);
	
	band_off = (off_t)nrows * ncols * bytes * (band - 1) + hbytes;

	for (row = 0; row < grass_nrows; row++) {
	    G_percent(row, nrows, 2);

	    if (flip & FLIP_V) {
		G_fseek(fp, (off_t) (grass_nrows - row - 1) * ncols * bytes + band_off,
			SEEK_SET);
	    }

	    if (fread(in_buf, bytes, ncols, fp) != ncols)
		G_fatal_error(_("Error reading data"));

	    convert_row(out_buf, in_buf, ncols, is_fp, is_signed,
			bytes, swap_flag, null_val, flip);

	    Rast_put_d_row(fd, out_buf);
	}

	G_percent(row, nrows, 2);	/* finish it off */

	Rast_close(fd);

	G_debug(1, "Creating support files for %s", output);

	if (title)
	    Rast_put_cell_title(output, title);

	Rast_short_history(output, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(output, &history);
    }

    fclose(fp);

    return EXIT_SUCCESS;
}
