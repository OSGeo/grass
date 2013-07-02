
/****************************************************************************
 *
 * MODULE:       r.volume
 * AUTHOR(S):    Dr. James Hinthorne, Central Washington University GIS Laboratory
 *               December 1988, (revised April 1989) (original contributor)
 *               Revised Jul 1995 to use new sites API (McCauley)
 *               GRASS 6 update: Hamish Bowman <hamish_b yahoo.com>
 *               Glynn Clements <glynn gclements.plus.com>, Soeren Gebbert <soeren.gebbert gmx.de>
 * PURPOSE:      
 *               r.volume is a program to compute the total, and average of cell values
 *               within regions of a map defined by clumps or patches on a second map
 *               (or MASK).  It also computes the "volume" by multiplying the total
 *               within a clump by the area of each cell.  It also outputs the
 *               "centroid" location of each clump.  Output is to standard out.
 *
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/site.h>
#include <grass/glocale.h>

int centroids(int, int *, int *, int, int);

int main(int argc, char *argv[])
{
    /* variables   */
    DCELL *data_buf;
    CELL *clump_buf;
    CELL i, max;
    int row, col, rows, cols;
    int out_mode, use_MASK, *n, *e;
    long int *count;
    int fd_data, fd_clump;
    const char *datamap, *clumpmap, *site_list;
    const char *clump_mapset;
    double avg, vol, total_vol, east, north, *sum;
    struct Cell_head window;
    struct Map_info *fd_sites = NULL;
    Site *mysite;
    Site_head site_info;
    struct GModule *module;
    struct Option *opt1, *opt2, *opt3;
    struct Flag *flag1;

    /* Initialize GIS */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("volume"));
    module->description =
	_("Calculates the volume of data \"clumps\", "
	  "and (optionally) produces a GRASS vector points map "
	  "containing the calculated centroids of these clumps.");

    opt1 = G_define_standard_option(G_OPT_R_INPUT);
    opt1->description =
	_("Existing raster map representing data that will be summed within clumps");

    opt2 = G_define_standard_option(G_OPT_R_INPUT);
    opt2->key = "clump";
    opt2->required = NO;
    opt2->description =
	_("Existing raster map, preferably the output of r.clump");

    opt3 = G_define_standard_option(G_OPT_V_OUTPUT);
    opt3->key = "centroids";
    opt3->required = NO;
    opt3->description = _("Vector points map to contain clump centroids");

    flag1 = G_define_flag();
    flag1->key = 'f';
    flag1->description = _("Generate unformatted report");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* get current window */
    G_get_window(&window);

    /* initialize */
    out_mode = 1;		/* assume full output text */
    mysite = G_site_new_struct(CELL_TYPE, 2, 0, 4);

    /* get arguments */
    datamap = opt1->answer;

    if (opt2->answer)
	clumpmap = opt2->answer;
    else
	clumpmap = "";

    if (opt3->answer)
	site_list = opt3->answer;
    else
	site_list = "";

    out_mode = (!flag1->answer);

    if (*datamap == 0)
	G_fatal_error(_("No data map specified"));

    /*
     * See if MASK or a separate "clumpmap" layer is to be used-- it must(!)
     * be one of those two choices.
     */
    use_MASK = 0;
    if (*clumpmap == '\0') {
	clumpmap = "MASK";
	use_MASK = 1;
    }
    fd_data = Rast_open_old(datamap, "");
    if (use_MASK)
	clump_mapset = G_mapset();
    else
	clump_mapset = "";

    fd_clump = Rast_open_old(clumpmap, clump_mapset);

    /* initialize sites file (for centroids) if needed */
    if (*site_list) {
	fd_sites = G_fopen_sites_new(site_list);
	if (fd_sites == NULL)
	    G_fatal_error(_("Unable to open centroids vector points map"));
    }

    /* initialize data accumulation arrays */
    max = Rast_get_max_c_cat(clumpmap, clump_mapset);

    sum = (double *)G_malloc((max + 1) * sizeof(double));
    count = (long int *)G_malloc((max + 1) * sizeof(long int));

    for (i = 0; i <= max; i++) {
	sum[i] = 0;
	count[i] = 0;
    }

    data_buf = Rast_allocate_d_buf();
    clump_buf = Rast_allocate_c_buf();

    /* get window size */
    rows = window.rows;
    cols = window.cols;

    /* now get the data -- first pass */
    for (row = 0; row < rows; row++) {
	G_percent(row, rows, 2);
	Rast_get_d_row(fd_data, data_buf, row);
	Rast_get_c_row(fd_clump, clump_buf, row);
	for (col = 0; col < cols; col++) {
	    i = clump_buf[col];
	    if (i > max)
		G_fatal_error(
		    "Row=%d Col=%d Cat=%d in clump map [%s]; max=%d.\n"
		    "Cat value > max returned by Rast_get_max_c_cat.",
		    row, col, i, clumpmap, max);
	    if (i < 1)
		continue;	/* ignore zeros and negs */
	    count[i]++;
	    if (!Rast_is_d_null_value(&data_buf[col]))
		sum[i] += data_buf[col];
	}
    }
    G_percent(row, rows, 2);
    /* free some buffer space */
    G_free(data_buf);
    G_free(clump_buf);

    /* data lists for centroids of clumps */
    e = (int *)G_malloc((max + 1) * sizeof(int));
    n = (int *)G_malloc((max + 1) * sizeof(int));

    i = centroids(fd_clump, e, n, 1, max);

    /* got everything, now do output */
    if (*site_list) {
	/* FIXME: convert to modern vector points map metadata */
	char desc[GNAME_MAX * 2 + 40];

	site_info.form = NULL;
	site_info.time = NULL;
	site_info.stime = NULL;
	sprintf(desc, "from %s on map %s using clumps from %s",
		argv[0], datamap, clumpmap);
	site_info.desc = G_store(desc);
	site_info.name = G_store(site_list);
	site_info.labels =
	    G_store("centroid east|centroid north|#cat vol avg t n");
	G_site_put_head(fd_sites, &site_info);
    }
    if (out_mode) {
	fprintf(stdout, "Volume report on data from %s", datamap);
	fprintf(stdout, " using clumps on %s map\n\n", clumpmap);
	fprintf(stdout,
		" Cat    Average   Data   # Cells        Centroid             Total\n");
	fprintf(stdout,
		"Number  in clump  Total  in clump   Easting   Northing       Volume\n\n");
    }
    total_vol = 0.0;

    for (i = 1; i <= max; i++) {
	if (count[i]) {
	    avg = sum[i] / (double)count[i];
	    vol = sum[i] * window.ew_res * window.ns_res;
	    total_vol += vol;
	    east = window.west + (e[i] + 0.5) * window.ew_res;
	    north = window.north - (n[i] + 0.5) * window.ns_res;
	    if (*site_list) {
		mysite->east = east;
		mysite->north = north;
		mysite->ccat = i;
		mysite->dbl_att[0] = vol;
		mysite->dbl_att[1] = avg;
		mysite->dbl_att[2] = sum[i];
		mysite->dbl_att[3] = (double)count[i];
		/*       "%-1.2f|%-1.2f|#%5d v=%-1.2f a=%-1.2f t=%-1.0f n=%ld\n", */
		/* east, north, i, vol, avg, sum[i], count[i]); */
		G_site_put(fd_sites, mysite);
	    }
	    if (out_mode)
		fprintf(stdout,
			"%5d%10.2f%10.0f %7ld  %10.2f  %10.2f %16.2f\n", i,
			avg, sum[i], count[i], east, north, vol);
	    else
		fprintf(stdout, "%d:%.2f:%.0f:%ld:%.2f:%.2f:%.2f\n",
			i, avg, sum[i], count[i], east, north, vol);
	}
    }
    if (total_vol > 0.0 && out_mode)
	fprintf(stdout, "%58s %14.2f", "Total Volume =", total_vol);
    fprintf(stdout, "\n");
    exit(EXIT_SUCCESS);
}				/* end of main() */
