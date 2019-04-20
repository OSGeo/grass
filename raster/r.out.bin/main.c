/*
 *   r.out.bin
 *
 *   Copyright (C) 2000,2010 by the GRASS Development Team
 *   Author: Bob Covill <bcovill@tekmap.ns.ca>
 *   Modified by Glynn Clements, 2010-01-10
 *
 *   This program is free software under the GPL (>=v2)
 *   Read the file COPYING coming with GRASS for details.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "gmt_grd.h"

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

static void write_int(FILE *fp, int swap_flag, int x)
{
    if (swap_flag)
	swap_4(&x);

    if (fwrite(&x, 4, 1, fp) != 1)
	G_fatal_error(_("Error writing data"));
}

static void write_double(FILE *fp, int swap_flag, double x)
{
    if (swap_flag)
	swap_8(&x);

    if (fwrite(&x, 8, 1, fp) != 1)
	G_fatal_error(_("Error writing data"));
}

static void make_gmt_header(
    struct GRD_HEADER *header,
    const char *name, const char *outfile,
    const struct Cell_head *region, double null_val)
{
    struct FPRange range;
    DCELL z_min, z_max;

    Rast_read_fp_range(name, "", &range);
    Rast_get_fp_range_min_max(&range, &z_min, &z_max);

    header->nx = region->cols;
    header->ny = region->rows;
    header->node_offset = 1;	/* 1 is pixel registration */
    header->x_min = region->west;
    header->x_max = region->east;
    header->y_min = region->south;
    header->y_max = region->north;
    header->z_min = (double) z_min;
    header->z_max = (double) z_max;
    header->x_inc = region->ew_res;
    header->y_inc = region->ns_res;
    header->z_scale_factor = 1.0;
    header->z_add_offset = 0.0;

    if (region->proj == PROJECTION_LL) {
	strcpy(header->x_units, "degrees");
	strcpy(header->y_units, "degrees");
    }
    else {
	strcpy(header->x_units, "Meters");
	strcpy(header->y_units, "Meters");
    }

    strcpy(header->z_units, "elevation");
    strcpy(header->title, name);
    sprintf(header->command, "r.out.bin -h input=%s output=%s", name, outfile);
    sprintf(header->remark, "%g used for NULL", null_val);
}

static void write_gmt_header(const struct GRD_HEADER *header, int swap_flag, FILE *fp)
{
    /* Write Values 1 at a time if byteswapping */
    write_int(fp, swap_flag, header->nx);
    write_int(fp, swap_flag, header->ny);
    write_int(fp, swap_flag, header->node_offset);

    write_double(fp, swap_flag, header->x_min);
    write_double(fp, swap_flag, header->x_max);
    write_double(fp, swap_flag, header->y_min);
    write_double(fp, swap_flag, header->y_max);
    write_double(fp, swap_flag, header->z_min);
    write_double(fp, swap_flag, header->z_max);
    write_double(fp, swap_flag, header->x_inc);
    write_double(fp, swap_flag, header->y_inc);
    write_double(fp, swap_flag, header->z_scale_factor);
    write_double(fp, swap_flag, header->z_add_offset);

    fwrite(header->x_units, sizeof(char[GRD_UNIT_LEN]),    1, fp);
    fwrite(header->y_units, sizeof(char[GRD_UNIT_LEN]),    1, fp);
    fwrite(header->z_units, sizeof(char[GRD_UNIT_LEN]),    1, fp);
    fwrite(header->title,   sizeof(char[GRD_TITLE_LEN]),   1, fp);
    fwrite(header->command, sizeof(char[GRD_COMMAND_LEN]), 1, fp);
    fwrite(header->remark,  sizeof(char[GRD_REMARK_LEN]),  1, fp);
}

static void write_bil_hdr(
    const char *outfile, const struct Cell_head *region,
    int bytes, int order, int header, double null_val)
{
    char out_tmp[GPATH_MAX];
    FILE *fp;

    sprintf(out_tmp, "%s.hdr", outfile);
    G_verbose_message(_("Header File = %s"), out_tmp);

    /* Open Header File */
    fp = fopen(out_tmp, "w");
    if (!fp)
	G_fatal_error(_("Unable to create file <%s>"), out_tmp);

    fprintf(fp, "nrows %d\n", region->rows);
    fprintf(fp, "ncols %d\n", region->cols);
    fprintf(fp, "nbands 1\n");
    fprintf(fp, "nbits %d\n", bytes * 8);
    fprintf(fp, "byteorder %s\n", order == 0 ? "M" : "I");
    fprintf(fp, "layout bil\n");
    fprintf(fp, "skipbytes %d\n", header ? 892 : 0);
    fprintf(fp, "nodata %g\n", null_val);

    fclose(fp);
}

static void convert_cell(
    unsigned char *out_cell, const DCELL in_cell,
    int is_fp, int bytes, int swap_flag)
{
    if (is_fp) {
	switch (bytes) {
	case 4:
	    *(float *) out_cell = (float) in_cell;
	    break;
	case 8:
	    *(double *) out_cell = (double) in_cell;
	    break;
	}
    }
    else {
	switch (bytes) {
	case 1:
	    *(unsigned char *) out_cell = (unsigned char) in_cell;
	    break;
	case 2:
	    *(short *) out_cell = (short) in_cell;
	    break;
	case 4:
	    *(int *) out_cell = (int) in_cell;
	    break;
#ifdef HAVE_LONG_LONG_INT
	case 8:
	    *(long long *) out_cell = (long long) in_cell;
	    break;
#endif
	}
    }

    if (swap_flag) {
	switch (bytes) {
	case 1:				break;
	case 2:	swap_2(out_cell);	break;
	case 4:	swap_4(out_cell);	break;
	case 8:	swap_8(out_cell);	break;
	}
    }
}

static void convert_row(
    unsigned char *out_buf, const DCELL *raster, int ncols,
    int is_fp, int bytes, int swap_flag, double null_val)
{
    unsigned char *ptr = out_buf;
    int i;

    for (i = 0; i < ncols; i++) {
	DCELL x = Rast_is_d_null_value(&raster[i])
	    ? null_val
	    : raster[i];
	convert_cell(ptr, x, is_fp, bytes, swap_flag);
	ptr += bytes;
    }
}

static void write_bil_wld(const char *outfile, const struct Cell_head *region)
{
    char out_tmp[GPATH_MAX];
    FILE *fp;

    sprintf(out_tmp, "%s.wld", outfile);
    G_verbose_message(_("World File = %s"), out_tmp);

    /* Open World File */
    fp = fopen(out_tmp, "w");
    if (!fp)
	G_fatal_error(_("Unable to create file <%s>"), out_tmp);

    fprintf(fp, "%f\n", region->ew_res);
    fprintf(fp, "0.0\n");
    fprintf(fp, "0.0\n");
    fprintf(fp, "-%f\n", region->ns_res);
    fprintf(fp, "%f\n", region->west + (region->ew_res / 2));
    fprintf(fp, "%f\n", region->north - (region->ns_res / 2));

    fclose(fp);
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
	struct Option *order;
    } parm;
    struct
    {
	struct Flag *int_out;
	struct Flag *float_out;
	struct Flag *gmt_hd;
	struct Flag *bil_hd;
	struct Flag *swap;
    } flag;
    char *name;
    char *outfile;
    double null_val;
    int do_stdout;
    int is_fp;
    int bytes;
    int order;
    int swap_flag;
    struct Cell_head region;
    int nrows, ncols;
    DCELL *in_buf;
    unsigned char *out_buf;
    int fd;
    FILE *fp;
    struct GRD_HEADER header;
    int row;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("export"));
    G_add_keyword(_("output"));
    module->description = _("Exports a GRASS raster to a binary array.");

    /* Define the different options */

    parm.input = G_define_option();
    parm.input->key = "input";
    parm.input->type = TYPE_STRING;
    parm.input->required = YES;
    parm.input->gisprompt = "old,cell,raster";
    parm.input->description = _("Name of input raster map");

    parm.output = G_define_option();
    parm.output->key = "output";
    parm.output->type = TYPE_STRING;
    parm.output->required = NO;
    parm.output->description =
	_("Name for output binary map (use output=- for stdout)");

    parm.null = G_define_option();
    parm.null->key = "null";
    parm.null->type = TYPE_DOUBLE;
    parm.null->required = NO;
    parm.null->answer = "0";
    parm.null->description = _("Value to write out for null");

    parm.bytes = G_define_option();
    parm.bytes->key = "bytes";
    parm.bytes->type = TYPE_INTEGER;
    parm.bytes->required = NO;
    parm.bytes->options = "1,2,4,8";
    parm.bytes->description = _("Number of bytes per cell");

    parm.order = G_define_option();
    parm.order->key = "order";
    parm.order->type = TYPE_STRING;
    parm.order->required = NO;
    parm.order->options = "big,little,native,swap";
    parm.order->description = _("Output byte order");
    parm.order->answer = "native";

    flag.int_out = G_define_flag();
    flag.int_out->key = 'i';
    flag.int_out->description = _("Generate integer output");

    flag.float_out = G_define_flag();
    flag.float_out->key = 'f';
    flag.float_out->description = _("Generate floating-point output");

    flag.gmt_hd = G_define_flag();
    flag.gmt_hd->key = 'h';
    flag.gmt_hd->description = _("Export array with GMT compatible header");

    flag.bil_hd = G_define_flag();
    flag.bil_hd->key = 'b';
    flag.bil_hd->description = _("Generate BIL world and header files");

    flag.swap = G_define_flag();
    flag.swap->key = 's';
    flag.swap->description = _("Byte swap output");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (G_strcasecmp(parm.null->answer, "nan") == 0)
	Rast_set_d_null_value(&null_val, 1);
    else if (sscanf(parm.null->answer, "%lf", &null_val) != 1)
	G_fatal_error(_("Invalid value for null (integers only)"));

    name = parm.input->answer;

    if (parm.output->answer)
	outfile = parm.output->answer;
    else {
	outfile = G_malloc(strlen(name) + 4 + 1);
	sprintf(outfile, "%s.bin", name);
    }

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

    swap_flag = order == (G_is_little_endian() ? 0 : 1);

    do_stdout = strcmp("-", outfile) == 0;

    if (flag.int_out->answer && flag.float_out->answer)
	G_fatal_error(_("-%c and -%c are mutually exclusive"),
			flag.int_out->key, flag.float_out->key);

    fd = Rast_open_old(name, "");

    if (flag.int_out->answer)
	is_fp = 0;
    else if (flag.float_out->answer)
	is_fp = 1;
    else
	is_fp = Rast_get_map_type(fd) != CELL_TYPE;

    if (parm.bytes->answer)
	bytes = atoi(parm.bytes->answer);
    else if (is_fp)
	bytes = 4;
    else
	bytes = 2;

    if (is_fp && bytes < 4)
	G_fatal_error(_("Floating-point output requires %s=4 or %s=8"),
			parm.bytes->key, parm.bytes->key);

#ifndef HAVE_LONG_LONG_INT
    if (!is_fp && bytes > 4)
	G_fatal_error(_("Integer output doesn't support %s=8 in this build"),
			parm.bytes->key);
#endif

    G_get_window(&region);

    /* open bin file for writing */
    if (do_stdout)
	fp = stdout;
    else if (NULL == (fp = fopen(outfile, "w")))
	G_fatal_error(_("Unable to create file <%s>"), outfile);

    /* Set up Parameters for GMT header */
    if (flag.gmt_hd->answer) {
	if (!is_fp && bytes > 4)
	    G_fatal_error(_("GMT grid doesn't support 64-bit integers"));
	make_gmt_header(&header, name, outfile, &region, null_val);
    }

    /* Write out BIL support files compatible with Arc-View */
    if (flag.bil_hd->answer) {
	G_message(_("Creating BIL support files..."));
	write_bil_hdr(outfile, &region,
		      bytes, order, flag.gmt_hd->answer, null_val);
	write_bil_wld(outfile, &region);
    }

    /* Write out GMT Header if required */
    if (flag.gmt_hd->answer)
	write_gmt_header(&header, swap_flag, fp);

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    in_buf = Rast_allocate_d_buf();
    out_buf = G_malloc(ncols * bytes);

    if (is_fp) {
	G_message(_("Exporting raster as floating values (%s=%d)"),
			parm.bytes->key, bytes);
	if (flag.gmt_hd->answer)
	    G_message(_("Writing GMT float format ID=1"));
    }
    else {
	G_message(_("Exporting raster as integer values (%s=%d)"),
			parm.bytes->key, bytes);
	if (flag.gmt_hd->answer)
	    G_message(_("Writing GMT integer format ID=2"));
    }

    G_verbose_message(_("Using the current region settings..."));
    G_verbose_message(_("north=%f"), region.north);
    G_verbose_message(_("south=%f"), region.south);
    G_verbose_message(_("east=%f"), region.east);
    G_verbose_message(_("west=%f"), region.west);
    G_verbose_message(_("r=%d"), region.rows);
    G_verbose_message(_("c=%d"), region.cols);

    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);

	Rast_get_d_row(fd, in_buf, row);

	convert_row(out_buf, in_buf, ncols, is_fp, bytes, swap_flag, null_val);

	if (fwrite(out_buf, bytes, ncols, fp) != ncols)
	    G_fatal_error(_("Error writing data"));
    }

    G_percent(row, nrows, 2);	/* finish it off */

    Rast_close(fd);
    fclose(fp);

    return EXIT_SUCCESS;
}

