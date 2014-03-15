
/****************************************************************************
 *
 * MODULE:       r.surf.idw2
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor)
 *               Roberto Flor <flor itc.it>, Markus Neteler <neteler itc.it>,
 *               Glynn Clements <glynn gclements.plus.com>, Jachym Cepicky <jachym les-ejk.cz>,
 *               Jan-Oliver Wagner <jan intevation.de>, Radim Blazek <radim.blazek gmail.com>
 * PURPOSE:      
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "local_proto.h"
#include <grass/glocale.h>

int search_points = 12;

int npoints = 0;
int npoints_alloc = 0;
int nsearch;

struct Point
{
    double north, east;
    double z;
    double dist;
};
struct Point *points = NULL;
struct Point *list;


int main(int argc, char *argv[])
{
    int fd, maskfd;
    CELL *cell, *mask;
    struct Cell_head window;
    int row, col;
    double north, east;
    double dx, dy;
    double maxdist, dist;
    double sum1, sum2;
    int i, n, max;
    struct GModule *module;
    struct History history;
    struct
    {
	struct Option *input, *npoints, *output;
    } parm;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("surface"));
    G_add_keyword(_("interpolation"));
    G_add_keyword(_("IDW"));
    module->description = _("Surface generation program.");

    parm.input = G_define_standard_option(G_OPT_R_INPUT);

    parm.output = G_define_standard_option(G_OPT_R_OUTPUT);

    parm.npoints = G_define_option();
    parm.npoints->key = "npoints";
    parm.npoints->key_desc = "count";
    parm.npoints->type = TYPE_INTEGER;
    parm.npoints->required = NO;
    parm.npoints->description = _("Number of interpolation points");
    parm.npoints->answer = "12";

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* Make sure that the current projection is not lat/long */
    if ((G_projection() == PROJECTION_LL))
	G_fatal_error(_("Lat/long databases not supported by r.surf.idw2. Use r.surf.idw instead!"));

    if (sscanf(parm.npoints->answer, "%d", &search_points) != 1 ||
	search_points < 1)
	G_fatal_error(_("%s=%s - illegal number of interpolation points"),
		      parm.npoints->key, parm.npoints->answer);

    list = (struct Point *)G_calloc(search_points, sizeof(struct Point));

    /* read the elevation points from the input raster map */
    read_cell(parm.input->answer);

    if (npoints == 0)
	G_fatal_error(_("%s: no data points found"), G_program_name());
    nsearch = npoints < search_points ? npoints : search_points;

    /* get the window, allocate buffers, etc. */
    G_get_set_window(&window);

    cell = Rast_allocate_c_buf();

    if ((maskfd = Rast_maskfd()) >= 0)
	mask = Rast_allocate_c_buf();
    else
	mask = NULL;

    fd = Rast_open_c_new(parm.output->answer);

    G_message(_n("Interpolating raster map <%s>... %d row... ",
        "Interpolating raster map <%s>... %d rows... ", window.rows),
	      parm.output->answer, window.rows);

    north = window.north - window.ns_res / 2.0;
    for (row = 0; row < window.rows; row++) {
	G_percent(row, window.rows, 2);

	if (mask)
	    Rast_get_c_row(maskfd, mask, row);

	north += window.ns_res;
	east = window.west - window.ew_res / 2.0;
	for (col = 0; col < window.cols; col++) {
	    east += window.ew_res;
	    /* don't interpolate outside of the mask */
	    if (mask && mask[col] == 0) {
		cell[col] = 0;
		continue;
	    }
	    /* fill list with first nsearch points */
	    for (i = 0; i < nsearch; i++) {
		dy = points[i].north - north;
		dx = points[i].east - east;
		list[i].dist = dy * dy + dx * dx;
		list[i].z = points[i].z;
	    }
	    /* find the maximum distance */
	    maxdist = list[max = 0].dist;
	    for (n = 1; n < nsearch; n++) {
		if (maxdist < list[n].dist)
		    maxdist = list[max = n].dist;
	    }
	    /* go thru rest of the points now */
	    for (; i < npoints; i++) {
		dy = points[i].north - north;
		dx = points[i].east - east;
		dist = dy * dy + dx * dx;

		if (dist < maxdist) {
		    /* replace the largest dist */
		    list[max].z = points[i].z;
		    list[max].dist = dist;
		    maxdist = list[max = 0].dist;
		    for (n = 1; n < nsearch; n++) {
			if (maxdist < list[n].dist)
			    maxdist = list[max = n].dist;
		    }
		}
	    }

	    /* interpolate */
	    sum1 = 0.0;
	    sum2 = 0.0;
	    for (n = 0; n < nsearch; n++) {
		if ((dist = list[n].dist)) {
		    sum1 += list[n].z / dist;
		    sum2 += 1.0 / dist;
		}
		else {
		    sum1 = list[n].z;
		    sum2 = 1.0;
		    break;
		}
	    }
	    cell[col] = (CELL) (sum1 / sum2 + 0.5);
	}

	Rast_put_row(fd, cell, CELL_TYPE);
    }

    G_free(points);
    G_free(cell);
    Rast_close(fd);

    /* writing history file */
    Rast_short_history(parm.output->answer, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(parm.output->answer, &history);
    G_done_msg(" ");

    exit(EXIT_SUCCESS);
}

int newpoint(double z, double east, double north)
{
    if (npoints_alloc <= npoints) {
	npoints_alloc += 1024;
	points = (struct Point *)G_realloc(points,
					   npoints_alloc *
					   sizeof(struct Point));
    }
    points[npoints].north = north;
    points[npoints].east = east;
    points[npoints].z = z;
    npoints++;

    return 0;
}
