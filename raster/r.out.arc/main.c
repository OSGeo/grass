#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

/*
 ****************************************************************************
 *
 * MODULE:       r.out.arc
 * AUTHOR(S):    Original author: Michael Shapiro (r.out.ascii)
 *               modified to r.out.arc by Markus Neteler, Univ. of Hannover
 *               neteler geog.uni-hannover.de (11/99)
 * PURPOSE:      r.out.arc: writes ARC/INFO ASCII GRID file
 * COPYRIGHT:    (C) 2000 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 *****************************************************************************/

int main(int argc, char *argv[])
{
    void *raster, *ptr;

    /*
       char  *null_row;
     */
    RASTER_MAP_TYPE out_type, map_type;
    char *outfile;
    char null_str[80];
    char cell_buf[300];
    int fd;
    int row, col;
    int nrows, ncols, dp;
    int do_stdout;
    FILE *fp;
    double cellsize;
    struct GModule *module;
    struct
    {
	struct Option *map;
	struct Option *output;
	struct Option *dp;
	struct Option *null;
    } parm;
    struct
    {
	struct Flag *noheader;
	struct Flag *singleline;
	struct Flag *ccenter;
    } flag;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("export"));
    G_add_keyword("ASCII");
    module->description =
	_("Converts a raster map layer into an ESRI ARCGRID file.");

    /* Define the different options */
    parm.map = G_define_standard_option(G_OPT_R_INPUT);

    parm.output = G_define_standard_option(G_OPT_F_OUTPUT);
    parm.output->description =
	_("Name for output ARC-GRID file (use out=- for stdout)");

    parm.dp = G_define_option();
    parm.dp->key = "dp";
    parm.dp->type = TYPE_INTEGER;
    parm.dp->required = NO;
    parm.dp->answer = "8";
    parm.dp->description = _("Number of decimal places");

    flag.noheader = G_define_flag();
    flag.noheader->key = 'h';
    flag.noheader->description = _("Suppress printing of header information");

    /* Added to optionally produce a single line output.     -- emes -- 12.10.92 */
    flag.singleline = G_define_flag();
    flag.singleline->key = '1';
    flag.singleline->description =
	_("List one entry per line instead of full row");

    /* use cell center in header instead of cell corner */
    flag.ccenter = G_define_flag();
    flag.ccenter->key = 'c';
    flag.ccenter->description =
	_("Use cell center reference in header instead of cell corner");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    sscanf(parm.dp->answer, "%d", &dp);
    if (dp > 20 || dp < 0)
	G_fatal_error("dp has to be from 0 to 20");

    outfile = parm.output->answer;
    if ((strcmp("-", outfile)) == 0)
	do_stdout = 1;
    else
	do_stdout = 0;

    sprintf(null_str, "-9999");

    fd = Rast_open_old(parm.map->answer, "");

    map_type = Rast_get_map_type(fd);
    out_type = map_type;

    /*
       null_row = Rast_allocate_null_buf();
     */
    raster = Rast_allocate_buf(out_type);

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /* open arc file for writing */
    if (do_stdout)
	fp = stdout;
    else if (NULL == (fp = fopen(outfile, "w")))
	G_fatal_error(_("Unable to open file <%s>"), outfile);

    if (!flag.noheader->answer) {
	struct Cell_head region;
	char buf[128];

	G_get_window(&region);
	fprintf(fp, "ncols %d\n", region.cols);
	fprintf(fp, "nrows %d\n", region.rows);
	cellsize = fabs(region.east - region.west) / region.cols;

	if (G_projection() != PROJECTION_LL) {	/* Is Projection != LL (3) */
	    if (!flag.ccenter->answer) {
		G_format_easting(region.west, buf, region.proj);
		fprintf(fp, "xllcorner %s\n", buf);
		G_format_northing(region.south, buf, region.proj);
		fprintf(fp, "yllcorner %s\n", buf);
	    }
	    else {
		G_format_easting(region.west + cellsize / 2., buf, region.proj);
		fprintf(fp, "xllcenter %s\n", buf);
		G_format_northing(region.south + cellsize / 2., buf, region.proj);
		fprintf(fp, "yllcenter %s\n", buf);
	    }
	}
	else {			/* yes, lat/long */
	    G_format_easting(region.west, buf, -1);
	    fprintf(fp, "xllcorner %s\n", buf);

	    G_format_northing(region.south, buf, -1);
	    fprintf(fp, "yllcorner %s\n", buf);
	}

	G_format_resolution(cellsize, buf, -1);
	fprintf(fp, "cellsize %s\n", buf);
	fprintf(fp, "NODATA_value %s\n", null_str);
    }

    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);
	Rast_get_row(fd, raster, row, out_type);
	/*
	   Rast_get_null_value_row(fd, null_row, row);
	 */
	for (col = 0, ptr = raster; col < ncols; col++,
	     ptr = G_incr_void_ptr(ptr, Rast_cell_size(out_type))) {
	    if (!Rast_is_null_value(ptr, out_type)) {
		if (out_type == CELL_TYPE)
		    fprintf(fp, "%d", *((CELL *) ptr));

		else if (out_type == FCELL_TYPE) {
		    sprintf(cell_buf, "%.*f", dp, *((FCELL *) ptr));
		    G_trim_decimal(cell_buf);
		    fprintf(fp, "%s", cell_buf);
		}
		else if (out_type == DCELL_TYPE) {
		    sprintf(cell_buf, "%.*f", dp, *((DCELL *) ptr));
		    G_trim_decimal(cell_buf);
		    fprintf(fp, "%s", cell_buf);
		}
	    }
	    else
		fprintf(fp, "%s", null_str);

	    if (!flag.singleline->answer)
		fprintf(fp, " ");
	    else
		fprintf(fp, "\n");
	}

	if (!flag.singleline->answer)
	    fprintf(fp, "\n");

	/*
	   for (col = 0; col < ncols; col++)
	   fprintf (fp,"%d ", null_row[col]);
	   fprintf (fp,"\n");
	 */
    }

    /* make sure it got to 100% */
    G_percent(1, 1, 2);

    Rast_close(fd);
    fclose(fp);

    exit(EXIT_SUCCESS);
}
