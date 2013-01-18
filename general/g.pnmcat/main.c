/*
 * MODULE:       g.pnmcat
 * AUTHOR(S):    Glynn Clements
 * PURPOSE:      Concatenate PNM tiles into a single image, for use by NVIZ'
 *               "Max. Resolution PPM" option.
 * COPYRIGHT:    (C) 2010 by Glynn Clements
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

struct infile {
    FILE *fp;
    int width;
};

static void read_line(char *buf, int size, FILE *fp)
{
    for (;;) {
	if (!fgets(buf, size, fp))
	    G_fatal_error(_("Error reading PPM file"));

	if (buf[0] != '#')
	    return;
    }
}

static void read_header(FILE *fp, int *width, int *height)
{
    char buf[80];
    char magic;
    int maxval;

    read_line(buf, sizeof(buf), fp);

    if (sscanf(buf, "P%c", &magic) != 1)
	G_fatal_error(_("Invalid PPM file"));

    if (magic != '6')
	G_fatal_error(_("Unsupported PPM file (format = P6 required)"));

    read_line(buf, sizeof(buf), fp);

    if (sscanf(buf, "%d %d", width, height) != 2)
	G_fatal_error(_("Invalid PPM file"));

    read_line(buf, sizeof(buf), fp);

    if (sscanf(buf, "%d", &maxval) != 1)
	G_fatal_error(_("Invalid PPM file"));

    if (maxval != 255)
	G_fatal_error(_("Unsupported PPM file (maxval = 255 required)"));
}

static void open_files(const char *base, struct infile *infiles,
		       int row, int cols,
		       int *p_width, int *p_height)
{
    int r_width = 0;
    int r_height;
    int col;

    for (col = 0; col < cols; col++) {
	struct infile *infile = &infiles[col];
	char path[GPATH_MAX];
	int c_width, c_height;
	FILE *fp;

	sprintf(path, "%s_%d_%d.ppm",  base, row + 1, col + 1);

	fp = fopen(path, "rb");
	if (!fp)
	    G_fatal_error(_("File <%s> not found"), path);

	read_header(fp, &c_width, &c_height);

	r_width += c_width;

	if (col == 0)
	    r_height = c_height;
	else
	    if (c_height != r_height)
		G_fatal_error(_("File <%s> has wrong height (expected %d, got %d"),
			      path, r_height, c_height);

	infile->fp = fp;
	infile->width = c_width;
    }

    *p_width = r_width;
    *p_height = r_height;
}

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct
    {
	struct Option *base, *out, *rows, *cols, *width, *height;
    } opt;
    int rows, cols, width, height;
    int row, col;
    int t_height = 0;
    FILE *out_fp;
    struct infile *infiles;
    char *buf;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("general"));
    module->description = "Concatenates multiple PPM image files";

    opt.base = G_define_option();
    opt.base->key = "base";
    opt.base->type = TYPE_STRING;
    opt.base->required = YES;
    opt.base->description = _("Base name of input files");

    opt.out = G_define_standard_option(G_OPT_F_OUTPUT);
    opt.out->required = YES;
    
    opt.rows = G_define_option();
    opt.rows->key = "rows";
    opt.rows->type = TYPE_INTEGER;
    opt.rows->required = YES;
    opt.rows->description = _("Number of rows");

    opt.cols = G_define_option();
    opt.cols->key = "cols";
    opt.cols->type = TYPE_INTEGER;
    opt.cols->required = YES;
    opt.cols->description = _("Number of columns");

    opt.width = G_define_option();
    opt.width->key = "width";
    opt.width->type = TYPE_INTEGER;
    opt.width->required = YES;
    opt.width->description = _("Image width");

    opt.height = G_define_option();
    opt.height->key = "height";
    opt.height->type = TYPE_INTEGER;
    opt.height->required = YES;
    opt.height->description = _("Image height");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    rows = atoi(opt.rows->answer);
    cols = atoi(opt.cols->answer);
    width = atoi(opt.width->answer);
    height = atoi(opt.height->answer);

    infiles = G_malloc(cols * sizeof(struct infile));
    buf = G_malloc(width * 3);

    out_fp = fopen(opt.out->answer, "wb");
    if (!out_fp)
	G_fatal_error(_("Unable to open output file <%s>"), opt.out->answer);

    fprintf(out_fp, "P6\n%d %d\n255\n", width, height);

    for (row = 0; row < rows; row++) {
	int r_width = 0, r_height;
	int i;

	open_files(opt.base->answer, infiles, rows - 1 - row, cols, &r_width, &r_height);

	if (r_width != width)
	    G_fatal_error(_("Row <%d> has wrong width (expected %d, got %d"),
			  row, width, r_width);

	t_height += r_height;
	if (t_height > height)
	    G_fatal_error(_("Invalid height (expected %d, got %d"),
			  height, t_height);

	for (i = 0; i < r_height; i++) {
	    char *p = buf;
	    for (col = 0; col < cols; col++) {
		struct infile *infile = &infiles[col];
		if (fread(p, 3, infile->width, infile->fp) != infile->width)
		    G_fatal_error(_("Error reading PPM file for tile <%d,%d> at row <%d>"),
				  row, col, i);
		p += 3 * infile->width;
	    }

	    if (fwrite(buf, 3, width, out_fp) != width)
		G_fatal_error(_("Error writing PPM file for tile row <%d> at row <%d>"),
			      row, i);
	}

	for (col = 0; col < cols; col++)
	    fclose(infiles[col].fp);
    }

    if (t_height != height)
	G_fatal_error(_("Incorrect height (expected %d, got %d"),
		      height, t_height);

    fclose(out_fp);

    return 0;
}

